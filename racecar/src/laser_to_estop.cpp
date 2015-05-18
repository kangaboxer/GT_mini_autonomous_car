#include "ros/ros.h"
#include "racecar/EmergencyStop.h"
#include "sensor_msgs/LaserScan.h"
#include <algorithm>


ros::Publisher estop_pub;
double prev_range=1;
ros::Time prev_time;
double filtered_dist=1.0;
double filtered_speed=0.0;
double filter_alpha=0.8;

void scanCallback(const sensor_msgs::LaserScan::ConstPtr& msg){
	sensor_msgs::LaserScan::_ranges_type::const_iterator min_it=std::min_element(msg->ranges.begin(),msg->ranges.end());
	double min_dist=*min_it;
	double min_angle=msg->angle_min+std::distance(msg->ranges.begin(),min_it)*msg->angle_increment;
	//ROS_INFO("closest obstacle at %f meters, in direction %f degrees",min_dist,min_angle/3.14159*180);
	if(min_dist<0.40){
		ROS_WARN("Emergency stop from laserdata : closest obstacle at %f meters, in direction %f degrees",min_dist,min_angle/3.14159*180);		
		racecar::EmergencyStop es_msg;
		es_msg.estop=true;
		es_msg.values.push_back(0);
		if(min_angle>0){
			es_msg.values.push_back(1);
		}else{
			es_msg.values.push_back(-1);
		}
		estop_pub.publish(es_msg);				
	}
	double closing_speed=(min_dist-prev_range)/((msg->header.stamp - prev_time).toSec());
	//ROS_INFO("closing speed %f m/s" , closing_speed);
	filtered_dist=filter_alpha*filtered_dist+(1-filter_alpha)*min_dist;
	filtered_speed=filter_alpha*filtered_speed+(1-filter_alpha)*closing_speed;
	double impact_time=-filtered_dist/filtered_speed;
//	double impact_time=-min_dist/closing_speed;
	if(impact_time>0 & impact_time<1.5){
		ROS_WARN("Emergency stop from laserdata : impact time in %f seconds",impact_time);		
		racecar::EmergencyStop es_msg;
		es_msg.estop=true;
		es_msg.values.push_back(0);
		if(min_angle>0){
			es_msg.values.push_back(1);
		}else{
			es_msg.values.push_back(-1);
		}
		estop_pub.publish(es_msg);		
	}
	prev_range=min_dist;
	prev_time=msg->header.stamp;
}

int main(int argc, char **argv)
{
  ros::init(argc, argv, "laser_to_estop");
  ros::NodeHandle n;
  estop_pub = n.advertise<racecar::EmergencyStop>("/emergencyStop", 10);
  prev_time=ros::Time::now();
  ros::Subscriber sub = n.subscribe("/scan", 1, scanCallback);
  ros::spin();
  return 0;
}