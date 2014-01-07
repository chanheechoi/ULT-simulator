#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "BeaconDeploy.h"

#define USE_MATH_DEFINES

#define RAND_WIDTH(r) (r * width - (width / 2))
#define RAND_LENGTH(r) (r * length - (length / 2))
#define RAND_HEIGHT(r) (r * (height / 2) + (height / 2))
#define MIN(a,b) (a<b?a:b)

void BeaconDeploy::deployBeacons(SimulatorArgument *args)
{
	this->args = args;
	args->beacons.reset();

	switch (args->deployType)
	{
	case SIM_BEACON::BYFILE :
		deployByFile();
		break;

	case SIM_BEACON::CIRCULAR :
		deployCircular();
		break;

	case SIM_BEACON::CIRCULAR2 :
		deployCircular2();
		break;

	case SIM_BEACON::COULOMB :
		deployCoulomb();
		break;

	case SIM_BEACON::RANDOM :
		deployRandom();
		break;

	default:
		printf("BeaconDeploy::deployBeacons, unknown deploy type\n");
		exit(30);
	}
}

void BeaconDeploy::deployByFile()
{
	Setting.loadBeaconList(args->beaconDeployFilename, &args->beacons);
}

void BeaconDeploy::deployCircular()
{
	int bid = 1;
	for (int i = 0; i < args->beaconSize; i++)
	{
		double baseAngle = 2 * M_PI / (double) args->beaconSize;
		double rAngle = baseAngle * i;
		double rDistance = args->width/2.0 * 1.0 / 2.0;
		Vector location;
		location = Vector(cos(rAngle) * rDistance, sin(rAngle) * rDistance, args->height);

		args->beacons.addBeacon(bid++, location);
	}
}

void BeaconDeploy::deployCircular2()
{
	int bid = 1;
	int innerSize = args->beaconSize / 3;
	int outterSize = args->beaconSize - innerSize;

	if (innerSize == 0)
	{
		args->beacons.addBeacon(bid++, Vector(0, 0, args->height));
	}
	else
	{
		for (int i = 0; i < innerSize; i++)
		{
			double baseAngle = 2 * M_PI / (double) innerSize;
			double rAngle = baseAngle * i;
			double rDistance = args->width/2.0 * 1.0 / 3.0;
			Vector location = Vector(cos(rAngle) * rDistance, sin(rAngle) * rDistance, args->height);

			args->beacons.addBeacon(bid++, location);
		}
	}

	double outterOffset;
	if (innerSize == 0) 
		outterOffset = 0;
	else
		outterOffset = 2 * M_PI / (double) innerSize / 2.0;

	for (int i = 0; i < outterSize; i++)
	{
		double baseAngle = 2 * M_PI / (double) outterSize + outterOffset;
		double rAngle = baseAngle * i;
		double rDistance = args->width/2.0 * 2.0 / 3.0;
		Vector location = Vector(cos(rAngle) * rDistance, sin(rAngle) * rDistance, args->height);

		args->beacons.addBeacon(bid++, location);
	}
}

void BeaconDeploy::deployCoulomb()
{
	std::vector<Vector> vBeacon;


	vBeacon.push_back(Vector(0, 0, 0));
	for (int i = 1; i < args->beaconSize; i++)
	{
		double baseAngle = 2 * M_PI / (double) (args->beaconSize - 1);
		double rAngle = baseAngle * i;
		double rDistance = args->width/2.0 * 1.0 / 2.0 / 2;
		vBeacon.push_back(Vector(cos(rAngle) * rDistance, sin(rAngle) * rDistance, args->height));
	}
	
	const int qPlane = 20;
	const int qBeacon = 20;
	const int k = 1000;
	bool stable = false;
	do
	{
		stable = true;
		for (int i = 0; i < args->beaconSize; i++)
		{
			Vector vForce = Vector(0, 0, 0);
			Vector vNorm;
			double distance;
			for (size_t j = 0; j < args->planes.size(); j++)
			{
				Plane *plane = args->planes.at(j);
				vNorm = (vBeacon[i] - plane->getReflectedPoint(vBeacon[i])).getUnitVector();
				distance = plane->getDistanceToPoint(vBeacon[i]);
				if (distance < 1) continue;
				vForce = vForce + (vNorm * (k * qPlane * qBeacon / (distance * distance)));
			}
			for (int j = 0; j < args->beaconSize; j++)
			{
				if (i == j) continue;
				vNorm = (vBeacon[i] - vBeacon[j]).getUnitVector();
				distance = vBeacon[i].getDistance(vBeacon[j]);
				if (distance < 1) continue;
				vForce = vForce + (vNorm * (k * qBeacon * qBeacon / (distance * distance)));
			}
			vForce.z = 0;
			if (vForce.getSize() > 0.5) stable = false;

			bool inside;
			do
			{
				inside = true;
				Vector vExpected = vBeacon[i] + vForce;
				for (size_t j =0; j < args->planes.size(); j++)
				{
					if (args->planes.at(j)->getSign(vExpected) > 0)
					{
						inside = false; 
						break;
					}
				}
				vForce = vForce * 0.5;
			}while(!inside);
			vBeacon[i] = vBeacon[i] + vForce;
//			printf("%.3f ", vForce.getSize());
		}
	}while(!stable);	

	int bid = 1;
	for (int i = 0; i < args->beaconSize; i++)
	{
		args->beacons.addBeacon(bid++, vBeacon[i]);
	}
}


void BeaconDeploy::deployCoulomb2()
{
	std::vector<Vector> vBeacon;


	for (int i = 0; i < args->beaconSize; i++)
	{
		double baseAngle = 2 * M_PI / (double) (args->beaconSize);
		double rAngle = baseAngle * i;
		double rDistance = args->width/2.0 * 1.0 / 2.0 / 2;
		vBeacon.push_back(Vector(cos(rAngle) * rDistance, sin(rAngle) * rDistance, args->height));
	}

	const int qPlane = 60;
	const int qBeacon = 40;
	const int k = 4000;
	bool stable = false;
	std::vector<Vector> vForces(args->beaconSize);

	do
	{
		stable = true;
		double fScaleFactor = 1.0;
		for (int i = 0; i < args->beaconSize; i++)
		{
			Vector vForce = Vector(0, 0, 0);
			Vector vNorm;
			double distance;
			for (size_t j = 0; j < args->planes.size(); j++)
			{
				Plane *plane = args->planes.at(j);
				vNorm = (vBeacon[i] - plane->getReflectedPoint(vBeacon[i])).getUnitVector();
				distance = plane->getDistanceToPoint(vBeacon[i]);
				if (distance < 1) continue;
				vForce = vForce + (vNorm * (k * qPlane * qBeacon / (distance * distance)));
			}
			for (int j = 0; j < args->beaconSize; j++)
			{
				if (i == j) continue;
				vNorm = (vBeacon[i] - vBeacon[j]).getUnitVector();
				distance = vBeacon[i].getDistance(vBeacon[j]);
				//				if (distance < 1) continue;
				vForce = vForce + (vNorm * (k * qBeacon * qBeacon / (distance * distance)));
			}
			vForce.z = 0;
			if (vForce.getSize() > 5.0) stable = false;

			vForces[i] = vForce;
			bool inside;
			do
			{
				inside = true;
				Vector vExpected = vBeacon[i] + (vForce * fScaleFactor);
				for (size_t j =0; j < args->planes.size(); j++)
				{
					if (args->planes.at(j)->getSign(vExpected) > 0)
					{
						inside = false; 
						break;
					}
				}
				if (!inside)
					fScaleFactor /= 1.2;				
			}while(!inside);
			//			vBeacon[i] = vBeacon[i] + vForce;
			//			printf("%.3f ", vForce.getSize());
		}

		for (int i = 0; i < args->beaconSize; i++)
		{
			vBeacon[i] = vBeacon[i] + (vForces[i] * fScaleFactor);
		}
	}while(!stable);	

	int bid = 1;
	for (int i = 0; i < args->beaconSize; i++)
	{
		args->beacons.addBeacon(bid++, vBeacon[i]);
	}
}

void BeaconDeploy::deployRandom()
{
	bool onCeiling = true;
	for (int i = 0; i < args->beaconSize; i++)
	{
		int plane = (int)(Random.getUniformDist()*5);

		double x = Random.getUniformDist(-args->width / 2.0, args->width / 2.0);
		double y = Random.getUniformDist(-args->length / 2.0, args->length / 2.0);
		double z = Random.getUniformDist(args->height / 2.0, args->height);


		if (onCeiling)
			plane = 0;

		Vector location;
		switch(plane)
		{
		case 0:
			z = args->height;
			break;
		case 1:
			y = args->length / 2.0;
			break;
		case 2:
			x = args->width / 2.0;
			break;
		case 3:
			y = -args->length / 2.0;
			break;
		case 4:
			x = -args->width / 2.0;
			break;
		}
		args->beacons.addBeacon(i + 1, Vector(x, y, z));
	}
}