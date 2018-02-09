/* 1) subscribe imutracker raw message
   2) transform geometry_msg/pose to tf
   3) transfer to windows via tcp
*/
#include "ros/ros.h"
#include "std_msgs/String.h"
#include "std_msgs/Float64.h"
#include <sstream>
#include "geometry_msgs/Quaternion.h"
#include "tf2/LinearMath/Quaternion.h"
#include "geometry_msgs/Quaternion.h"
#include "geometry_msgs/Transform.h"
#include "geometry_msgs/Pose.h"
#include "geometry_msgs/PoseArray.h"
#include "geometry_msgs/PoseStamped.h"
#include "geometry_msgs/Vector3.h"
#include "tf2_msgs/TFMessage.h"
#include "tf/tfMessage.h"
#include "tf/transform_datatypes.h"
#include "sensor_msgs/Imu.h"
#include "sensor_msgs/MagneticField.h"
#include <tf/transform_broadcaster.h>

#include <unistd.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "quaternion.c"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <iomanip> // setprecision
#include <sstream> // stringstream

#define MESSAGE_FREQ 10 //should be 50Hz

void error(const char *msg) {
    perror(msg);
    exit(0);
}

char *names[] = {"ThumbProx", "ThumbInter", "ThumbDist", "IndexProx", "IndexInter", "IndexDist", "MiddleProx", "MiddleInter", "MiddleDist", "RingProx", "RingInter", "RingDist", "PinkyProx", "PinkyInter", "PinkyDist"};

//char *names[] = {"Thumb", "Index", "Middle", "Ring", "Pinky"};

class Transformer {
public:
  char topic_message[10000] = "";
public:
  void chatterCallback(const geometry_msgs::PoseArray::ConstPtr& msg);
  char* getMessageValue();
};

std::string float2str(float x) {
  std::stringstream stream;
  stream << std::fixed << std::setprecision(2) << x;
  return stream.str();
}

void Transformer::chatterCallback(const geometry_msgs::PoseArray::ConstPtr& msg)
{
  memset(topic_message, 0, 10000);
  //char ** fields = new char *[17];
  for (int i = 0; i < 15; i++) {
    //for (int j = 0; j < 3; j++) {
      //int idx = 3* i + j;
    geometry_msgs::Pose pose = msg->poses[i]; 
    strcat(topic_message, "0,0,0,");
    strcat(topic_message, names[i]);
    strcat(topic_message, ",");
    strcat(topic_message, float2str(pose.position.x).c_str());
    strcat(topic_message, ",");
    strcat(topic_message, float2str(pose.position.y).c_str());
    strcat(topic_message, ",");
    strcat(topic_message, float2str(pose.position.z).c_str());
    strcat(topic_message, ",");
    strcat(topic_message, float2str(pose.orientation.x).c_str());
    strcat(topic_message, ",");
    strcat(topic_message, float2str(pose.orientation.y).c_str());
    strcat(topic_message, ",");
    strcat(topic_message, float2str(pose.orientation.z).c_str());
    strcat(topic_message, ",");
    strcat(topic_message, float2str(pose.orientation.w).c_str());
    strcat(topic_message, "\n");
  }  
  ROS_INFO("%s", topic_message);
}
char* Transformer::getMessageValue() {
    return topic_message;
}

int main(int argc, char **argv)
{
  ros::init(argc, argv, "bridge");
  ros::NodeHandle nh;

//  ros::Subscriber sub = n.subscribe("tac_glove_imutracker_raw", 1000, chatterCallback);
  ros::Rate loop_rate(MESSAGE_FREQ); // set the rate as defined in the macro MESSAGE_FREQ
  Transformer trans;
  ros::Subscriber sub = nh.subscribe("tac_glove_imutracker_raw", 1000, &Transformer::chatterCallback, &trans);  int sockfd, portno, n, choice = 1;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  char buffer[1000];
  bool echoMode = false;
  if (argc < 3) {
    fprintf(stderr,"Usage: $ rosrun comm_tcp client_node <hostname> <port> --arguments\nArguments:\n -e : Echo mode\n");
    exit(0);
  }
  if (argc > 3)
    if (strcmp(argv[3], "-e") == 0)
      echoMode = true;
  portno = atoi(argv[2]);
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");
  server = gethostbyname(argv[1]);
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,server->h_length);
  serv_addr.sin_port = htons(portno); 
 
  if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
  while(ros::ok()) {
    bzero(buffer,1000);
    strcpy(buffer, trans.getMessageValue());
    loop_rate.sleep();
    
    n = write(sockfd,buffer,strlen(buffer));
    if (n < 0) 
      error("ERROR writing to socket");
    if (echoMode) {
      bzero(buffer, 1000);
      n = read(sockfd,buffer,999);
      if (n < 0)
	error("ERROR reading reply");
      printf("%s\n", buffer);
    }
    ros::spinOnce();
  }
  return 0;

}

