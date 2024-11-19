/*********************************************************************************************************************
Copyright (c) 2020 RoboSense
All rights reserved

By downloading, copying, installing or using the software you agree to this license. If you do not agree to this
license, do not download, install, copy or use the software.

License Agreement
For RoboSense LiDAR SDK Library
(3-clause BSD License)

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the names of the RoboSense, nor Suteng Innovation Technology, nor the names of other contributors may be used
to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*********************************************************************************************************************/

#include "manager/node_manager.hpp"

#include <rs_driver/macro/version.hpp>
#include <signal.h>

#ifdef ROS_FOUND
#include <ros/ros.h>
#include <ros/package.h>
#elif ROS2_FOUND
#include <rclcpp/rclcpp.hpp>
#endif

using namespace robosense::lidar;

#ifdef ROS2_FOUND
std::mutex g_mtx;
std::condition_variable g_cv;
#endif

static void sigHandler(int sig)
{
  RS_MSG << "RoboSense-LiDAR-Driver is stopping....." << RS_REND;

#ifdef ROS_FOUND
  ros::shutdown();
#elif ROS2_FOUND
  g_cv.notify_all();
#endif
}

int main(int argc, char** argv)
{
  signal(SIGINT, sigHandler);  ///< bind ctrl+c signal with the sigHandler function

  RS_TITLE << "********************************************************" << RS_REND;
  RS_TITLE << "**********                                    **********" << RS_REND;
  RS_TITLE << "**********    RSLidar_SDK Version: v" << RSLIDAR_VERSION_MAJOR 
    << "." << RSLIDAR_VERSION_MINOR 
    << "." << RSLIDAR_VERSION_PATCH << "     **********" << RS_REND;
  RS_TITLE << "**********                                    **********" << RS_REND;
  RS_TITLE << "********************************************************" << RS_REND;

#ifdef ROS_FOUND
  ros::init(argc, argv, "rslidar_sdk_node", ros::init_options::NoSigintHandler);
#elif ROS2_FOUND
  rclcpp::init(argc, argv);
#endif

  std::string config_path;

#ifdef RUN_IN_ROS_WORKSPACE
   config_path = ros::package::getPath("rslidar_sdk");
#else
   config_path = (std::string)PROJECT_PATH;
#endif

  //  config_path += "/config/config.yaml";
  std::string rslidar_config_file = (std::string)PROJECT_PATH + "/config/config.yaml";
  ros::param::get("rslidar_config_file", rslidar_config_file);

#ifdef ROS_FOUND
  ros::NodeHandle priv_hh("~");
  std::string path;
  priv_hh.param("config_path", path, std::string(""));
#elif ROS2_FOUND
  std::shared_ptr<rclcpp::Node> nd = rclcpp::Node::make_shared("param_handle");
  std::string path = nd->declare_parameter<std::string>("config_path", "");
#endif

#if defined(ROS_FOUND) || defined(ROS2_FOUND)
  if (!path.empty())
  {
    config_path = path;
  }
#endif

  YAML::Node config;
  try
  {
    // config = YAML::LoadFile(config_path);
    config = YAML::LoadFile(rslidar_config_file);
    RS_INFO << "--------------------------------------------------------" << RS_REND;
    RS_INFO << "Config loaded from PATH:" << RS_REND;
    RS_INFO << config_path << RS_REND;
    RS_INFO << "--------------------------------------------------------" << RS_REND;
  }
  catch (...)
  {
    RS_ERROR << "The format of config file " << config_path << " is wrong. Please check (e.g. indentation)." << RS_REND;
    return -1;
  }

  std::string lidar_type = "";
  priv_hh.param("lidar_type", lidar_type, lidar_type);
  if (lidar_type != "")
    config["lidar"][0]["driver"]["lidar_type"] = lidar_type;

  int msop_port = -1;
  priv_hh.param("msop_port", msop_port, msop_port);
  if (msop_port != -1)
    config["lidar"][0]["driver"]["msop_port"] = msop_port;

  int difop_port = -1;
  priv_hh.param("difop_port", difop_port, difop_port);
  if (difop_port != -1)
    config["lidar"][0]["driver"]["difop_port"] = difop_port;

  std::string ros_frame_id = "";
  priv_hh.param("frame_id", ros_frame_id, ros_frame_id);
  if (ros_frame_id != "")
    config["lidar"][0]["ros"]["ros_frame_id"] = ros_frame_id;
  

  std::shared_ptr<NodeManager> demo_ptr = std::make_shared<NodeManager>();
  demo_ptr->init(config);
  demo_ptr->start();

  RS_MSG << "RoboSense-LiDAR-Driver is running....." << RS_REND;

#ifdef ROS_FOUND
  ros::spin();
#elif ROS2_FOUND
  std::unique_lock<std::mutex> lck(g_mtx);
  g_cv.wait(lck);
#endif

  return 0;
}
