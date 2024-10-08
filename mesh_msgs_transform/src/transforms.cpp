/*
 *  Software License Agreement (BSD License)
 *
 *  Robot Operating System code by the University of Osnabrück
 *  Copyright (c) 2015, University of Osnabrück
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   1. Redistributions of source code must retain the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 *   3. Neither the name of the copyright holder nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 *
 *  mesh_msgs_transforms.cpp
 *
 *  author: Sebastian Pütz <spuetz@uni-osnabrueck.de>
 */

#include "mesh_msgs_transform/transforms.h"
#include <Eigen/Dense>
#include <tf2_ros/transform_listener.h>
#include <geometry_msgs/msg/point.hpp>

namespace mesh_msgs_transform{

inline void vectorTfToEigen(
  const tf2::Vector3& tf_vec, 
  Eigen::Vector3d& eigen_vec)
{
  eigen_vec(0) = tf_vec[0];
  eigen_vec(1) = tf_vec[1];
  eigen_vec(2) = tf_vec[2];
}

inline void pointMsgToEigen(
  const geometry_msgs::msg::Point& gm_point,
  Eigen::Vector3d& eigen_point)
{
  eigen_point(0) = gm_point.x;
  eigen_point(1) = gm_point.y;
  eigen_point(2) = gm_point.z;
}

inline void pointEigenToMsg(
  const Eigen::Vector3d& eigen_point,
  geometry_msgs::msg::Point& gm_point)
{
  gm_point.x = eigen_point(0);
  gm_point.y = eigen_point(1);
  gm_point.z = eigen_point(2);
}

bool transformGeometryMeshNoTime(
    const std::string& target_frame,
    const mesh_msgs::msg::MeshGeometryStamped& mesh_in,
    const std::string& fixed_frame,
    mesh_msgs::msg::MeshGeometryStamped& mesh_out,
    const tf2_ros::Buffer& tf_buffer)
{
  geometry_msgs::msg::TransformStamped transform;
  try
  {
    transform = tf_buffer.lookupTransform(
      target_frame, tf2::TimePointZero, 
      mesh_in.header.frame_id, tf2::TimePointZero,
      fixed_frame);
  }
  catch(const tf2::TransformException& ex)
  {
   RCLCPP_ERROR(rclcpp::get_logger("mesh_msgs_transform"), "%s", ex.what());
   return false;
  }

  Eigen::Quaterniond rotation(
    transform.transform.rotation.w,
    transform.transform.rotation.x,
    transform.transform.rotation.y,
    transform.transform.rotation.z);
    
  Eigen::Translation3d translation( 
    transform.transform.translation.x,
    transform.transform.translation.y,
    transform.transform.translation.z );

  Eigen::Affine3d transformation = translation * rotation;

  if(&mesh_in != &mesh_out)
  {
    mesh_out.header = mesh_in.header;
    mesh_out.mesh_geometry.faces = mesh_in.mesh_geometry.faces;
  }

  mesh_out.mesh_geometry.vertices.resize(mesh_in.mesh_geometry.vertices.size());
  mesh_out.mesh_geometry.vertex_normals.resize(mesh_in.mesh_geometry.vertex_normals.size());

  // transform vertices
  for(size_t i = 0; i < mesh_in.mesh_geometry.vertices.size(); i++)
  {
    Eigen::Vector3d in_vec, out_vec;
    pointMsgToEigen(mesh_in.mesh_geometry.vertices[i], in_vec);
    out_vec = transformation * in_vec;
    pointEigenToMsg(out_vec, mesh_out.mesh_geometry.vertices[i]);
  }

  // rotate normals
  for(size_t i = 0; i < mesh_in.mesh_geometry.vertex_normals.size(); i++)
  {
    Eigen::Vector3d in_vec, out_vec;
    pointMsgToEigen(mesh_in.mesh_geometry.vertex_normals[i], in_vec);
    out_vec = transformation.rotation() * in_vec;
    pointEigenToMsg(out_vec, mesh_out.mesh_geometry.vertex_normals[i]);
  }

  mesh_out.header.frame_id = target_frame;
  mesh_out.header.stamp = mesh_in.header.stamp;;

  return true;
}

} /* namespace mesh_msgs_transforms */
