#define _USE_MATH_DEFINES
#include <math.h>
#include "ListenerMover.h"

PathInfo::PathInfo(Random *random)
{
	vPosition = Vector(0, 0, 0);
	uvFace = Vector(0, 0, 0);
	
	speed_avg = speed_dev = 0;
	angular_avg = angular_dev = 0;
	face_theta = face_phi = 0;
	this->random = random;
	
}

PathInfo::~PathInfo()
{
}

/*
void PathInfo::setFace(double theta, double phi)
{
	this->face_theta = theta;		// projected angle to xy-plane
	this->face_phi = phi;			// angle to z-axis
	face_unit = getFaceVector(face_theta, face_phi);
}
*/

void PathInfo::setFace(Vector vFace)
{
	this->uvFace = vFace.getUnitVector();
	this->face_phi = acos(uvFace.z);
	this->face_theta = acos(uvFace.x / Vector(uvFace.x, uvFace.y, 0).getSize());

	Vector test = getFaceVector(face_theta, face_phi);
/*
	if (test.x != vFace.x || test.y != vFace.y || test.z != vFace.z)
	{
		double a = test.x + 1;
//		exit(1);
	}
*/
}


// return face vector of listener repect to theta and phi.
Vector PathInfo::getFaceVector(double theta, double phi)
{
	return Vector(sin(phi) * cos(theta),
					sin(phi) * sin(theta),
					cos(phi));	
}

void PathInfo::setCoefficient(double speed_avg, double speed_dev,
								  double angular_avg, double angular_dev, double interval)
{
	this->speed_avg = speed_avg;
	this->speed_dev = speed_dev;
	this->angular_avg = angular_avg;
	this->angular_dev = angular_dev;
//	this->timestamp = timestamp;
	this->interval = interval;
}


void PathInfo::setStartPosition(Vector position)
{
	this->vPosition = this->vStartPosition = this->vDestination = position;

}

void PathInfo::setFinishPosition(Vector position)
{
	this->vDestination = position;
	this->uvDirection = (vDestination - vStartPosition).getUnitVector();
	thetaY = acos(uvDirection.x / (sqrt(pow(uvDirection.x, 2) + pow(uvDirection.y, 2))));

	Vector vFace = this->uvDirection;
	vFace.y = -vFace.y;
	vFace = vFace * Vector(0, 0, 1);
	vFace.y = -vFace.y;
	this->setFace(vFace);
}


								

int PathInfo::moveNext()
{
	if (vStartPosition.isEqual(vDestination)) return 0;
//1	uvDirection = (vDestination - vPosition).getUnitVector();
	Vector vOri = vPosition;	
	/* (cm) * ((ms / 1000) = s) * (((m/s) * 100) = (cm/s)) */
	Vector vDirection = uvDirection * (interval / 1000.0) * 
					(random->getGaussDist(speed_avg, speed_dev) * 100.0);
 	vPosition = vPosition + vDirection;
	face_phi += (random->getGaussDist(angular_avg, angular_dev)) * (interval / 1000.0);
	face_theta += (random->getGaussDist(angular_avg, angular_dev)) * (interval / 1000.0);
//	uvFace = getFaceVector(face_theta, face_phi);

	Vector vRemain = vDestination - vOri;
	if (fabs(vRemain.getUnitVector() & vDirection) > vRemain.getSize()) return 0;
	uvDirection = (vDestination - vPosition).getUnitVector();

	return 1;
/*1
	if ((vDestination - vPosition).getUnitVector().isEqual(
		(vDestination - (vPosition + vDirection)).getUnitVector()))
		return 1;
	return 0;
*/
	
}





/* ListenerMover Class */

ListenerMover::ListenerMover(SimulatorArgument *args)
{
	this->width = args->width;
	this->length = args->length;
	this->height = args->height;
	reset();
	this->args = args;
}

ListenerMover::~ListenerMover(void)
{
}


void ListenerMover::reset()
{
	bFirst = true;
	pathList.clear();
	timestamp = 0;
}

void ListenerMover::reset(int width, int length, int height)
{
	this->width = width;
	this->length = length;
	this->height = height;
	reset();
}

void ListenerMover::setPath(Vector point, double speed)
{
	PathInfo pathInfo(&args->random);
	pathInfo.setStartPosition(point);
	pathInfo.setCoefficient(speed, args->speedDev, args->angleAvg, 
		args->angleDev, args->timeslot);
	if (pathList.size() > 0)
		pathList[pathList.size() - 1].setFinishPosition(point);
	pathList.push_back(pathInfo);	
}

bool ListenerMover::moveNext(ListenerInfo &path)
{
	if (bFirst)
	{
		itPathList = pathList.begin();
		bFirst = false;
		vPosition = itPathList->vStartPosition;
		timestamp = 0;
		vArmAcc = Vector(0, 0, 0);
		vArmVel = itPathList->uvDirection * itPathList->speed_avg;
		vArmVel.z = 0;
		pArm = vPosition;
	}	

	while(!itPathList->moveNext())
	{
		itPathList++;		
		if (itPathList == pathList.end()) return false;
		itPathList->vPosition = vPosition;
	}
	vPosition = itPathList->vPosition;
	vFace = itPathList->uvFace;

	// theta = A * cos (2 * PI * t / T)
	// T = 1 / speed
	// period = 1 per 1m
	double theta = 2.0 / 3.0 * cos(2 * M_PI * timestamp / 1000  * itPathList->speed_avg);
	double x = sin(theta) * ARM_LENGTH;
	double y = 0;
	double z = -cos(theta) * ARM_LENGTH;	
	double xp, yp, zp;

	xp = cos(itPathList->thetaY) * x + sin(itPathList->thetaY) * y;
	yp = -sin(itPathList->thetaY) * x + cos(itPathList->thetaY) * y;
	zp = z;

	x = xp;
	y = yp;
	z = zp;
	if (itPathList->uvDirection.y > 0)
		y = -yp;


	pArm = Vector(x, y, z) + vPosition;


	timestamp += (long)args->timeslot;

	path = ListenerInfo(timestamp, pArm, vFace);
	return true;
	
}


Vector ListenerMover::getListenerPosition()
{
//	return vPosition;
	return pArm;
}

Vector ListenerMover::getListenerFace()
{
	return vFace;
}
