// Ass 3.cpp: A program using the TL-Engine

// Ass 3.cpp: A program using the TL-Engine

#include <TL-Engine.h>	// TL-Engine include file and namespace
#include <sstream>
#include <math.h>
using namespace tle;
using namespace std;

/*collision enum*/
enum collisionSide
{
	none, Xaxis, zAxis
};

//to be used for collision calculations
struct vector2D
{
	float x;
	float z;
};
collisionSide SphereToBoxCollision2D(float sphereOldX, float sphereOldZ, float sphereX, float sphereZ, float sphereRadius,
	float boxXPos, float boxZPos, float boxHorizRadius, float boxVertRadius);

bool SphereToSphereCollision2D(float sphere1XPos, float sphere1ZPos, float sphere1Rad,
	float sphere2XPos, float sphere2ZPos, float sphere2Radius);

bool PointToBoxCollision(float pointXPos, float pointZPos, float boxXPos, float boxZPos, float boxHorizRadius, float boxVertRadius);

void CollisionResolution(collisionSide collisionResponse, vector2D &momentum);


vector2D scalar(float s, vector2D v)
{
	return { s * v.x, s * v.z };
}

vector2D sumMovVectors(vector2D v1, vector2D v2, vector2D v3)
{
	return{ v1.x + v2.x + v3.x, v1.z + v2.z + v3.z };
}

vector2D momentum{ 0.0f, 0.0f };
vector2D thrust{ 0.0f, 0.0f };
vector2D drag{ 0.0f, 0.0f };
void main()
{
	// Create a 3D engine (using TLX engine here) and open a window for it
	I3DEngine* myEngine = New3DEngine(kTLX);
	myEngine->StartWindowed();

	// Add default folder for meshes and other media
	myEngine->AddMediaFolder("./Media");


	/**** Set up your scene here ****/
	EKeyCode quitKey = Key_Escape;
	EKeyCode accelerateKey = Key_W;
	EKeyCode brakeKey = Key_S;
	EKeyCode leftTurnKey = Key_A;
	EKeyCode rightTurnKey = Key_D;

	/*State enumerators*/
	enum race_States
	{
		COUNTDOWN, CHECK1, CHECK2, CHECK3, CHECK4, CHECK5, CHECK6, CHECK7, CHECK8, FINISH, WAITING
	};

	/*Non object ascociated Variables*/
	bool Isboosting = false;
	bool isOverheated = false;
	bool isTop = false;

	//car animation variables
	float maxUpTilt = 100.0f;
	float maxUpHover = 1.0f;
	float maxTiltSides = 10.0f;

	float upTiltIncrement = 1.0f;
	float HoverIncrement = 1.0f;

	float currentUpTilt = 0.0f;
	float currentUpHover = 0.0f;
	float currentTiltSides = 0.0f;

	/*Create Skybox*/
	float skyboxYPos = -960.0f; //only need a Y Position because the rest are zero
	IMesh * skyboxMesh = myEngine->LoadMesh("Skybox 07.x");
	IModel * skybox = skyboxMesh->CreateModel(0.0, skyboxYPos, 0.0);

	/*----Create UI Backdrop----*/
	ISprite* uiBackdrop = myEngine->CreateSprite("backdrop.jpg", 675, 660);
	ISprite* uiBackdrop2 = myEngine->CreateSprite("backdrop.jpg", 0, 660);

	/*Quad*/
	IMesh * crossMesh = myEngine->LoadMesh("cross.x");
	IModel * crossMod = crossMesh->CreateModel(0, -10, 0);
	crossMod->Scale(0.33);

	/*Load fonts to be used on the screen*/
	IFont* uiFont = myEngine->LoadFont("Verdana", 36);
	IFont* raceFont = myEngine->LoadFont("Comic Sans MS", 120);

	/*Create Floor*/
	IMesh * floorMesh = myEngine->LoadMesh("ground.x");
	IModel * groundFloor = floorMesh->CreateModel();

	//create data for the car
	IMesh * carMesh = myEngine->LoadMesh("race2.x");
	IMesh * enemyMesh = myEngine->LoadMesh("race3.x");

	struct Car //make each car a reusable struct with the same attributes
	{
		IModel * model;
		float	 thrustFactor;
		float	 reverseThrust;
		int		 currentSpeed;
		float	 carMatrix[4][4];
		race_States state = WAITING;
		int lap = 0;

	};

	//player controlled instance of the car
	Car playerCar;
	playerCar.model = carMesh->CreateModel(-10, 0, -70);

	playerCar.currentSpeed = 0;
	playerCar.thrustFactor = 0.05f;
	playerCar.reverseThrust = 0.01f;
	const float carDragCoeff = -0.0002f;
	//don't initialise the speed yet


	//cpu car directed by waypoints
	Car cpuCar;
	cpuCar.model = enemyMesh->CreateModel(10, 0, -70);
	cpuCar.currentSpeed = 70;
	cpuCar.state = WAITING;
	float cpuSpeedTable[20];
	int currentWayPoint = 0;

	/*Create dummy waypoint models for the ai car*/
	IMesh * dummyMesh = myEngine->LoadMesh("dummy.x");
	IModel * waypoint[26];
	float dummyXPos[26]{ 0, 0,   65,  50,   45,  50, 62, 62, 180, 190, 195, 180, 250, 250, 255, 260, 262, 250, 250, 250, 200, 145, 125, 100, 105, 55 };
	float dummyZPos[26]{ 0, 345, 350, 300 , 282, 200,20, 25,  25, 105, 255, 345, 350, 105, 100,  92,  81,  65, 50,	0,	-30, -30, -50, -50, -80, -60 };
	for (int i = 0; i < 26; i++)
	{
		waypoint[i] = dummyMesh->CreateModel();
		waypoint[i]->SetPosition(dummyXPos[i], 0, dummyZPos[i]);
	}

	/*Create dummy for animation of car*/
	IModel * tiltUpDummy = dummyMesh->CreateModel(-10, 0, -70);
	IModel * leanSidesDummy = dummyMesh->CreateModel(-10, 0, -70);
	IModel * hoverDummy = dummyMesh->CreateModel(-10, 0, -70);
	IModel * moveDummy = dummyMesh->CreateModel(-10, 0, -70);

	playerCar.model->AttachToParent(moveDummy);
	moveDummy->AttachToParent(hoverDummy);
	tiltUpDummy->AttachToParent(playerCar.model);

	playerCar.model->Move(10, 0, 70);
	moveDummy->Move(10, 0, 70);
	tiltUpDummy->Move(10, 0, 70);
	float moveDummyMatrix[4][4];


	/*place the end segments of walls*/
	IMesh * wallendMesh = myEngine->LoadMesh("IsleStraight.x");

	struct Isles
	{
		IModel * IsleModel;
		float xPos = -10;
		float yPos = 0;
		float zPos = 40;
	};

	/*----create wall segments----*/
	IMesh * wallSegMesh = myEngine->LoadMesh("Wall.x");

	struct Wall
	{
		IModel * wallSeg;
		int numSeg;
		float xPos = -10.5;
		float yPos = 0;
		float zPos = 46;
		float wallOfset = 4.0f;

	};

	/*-Walls and isles at the start/finish line-*/

	Isles startFinish[4]; //the first 4 isles nearest to the start line
	for (int i = 0, j = 0; i < 4; i++) //need two loop variables, one for the left side and one for the right side
	{
		startFinish[i].IsleModel = wallendMesh->CreateModel(startFinish->xPos, 0, startFinish->zPos); //create all of the wall ends
		if (i < 2) //create the left side of the wall
		{
			startFinish[i].IsleModel->SetZ(startFinish[i].zPos + (i * 13)); //move the first two segments and offset them accordingly in the Z
		}

		if (i > 1) //create the right side of the wall
		{
			//move the second two segments from the original position they were created, offset by appropriate X and Z
			startFinish[i].IsleModel->SetX(startFinish[i].xPos + 20);
			startFinish[i].IsleModel->SetZ(startFinish[i].zPos + (j * 13));
			j++;
		}

	}

	Wall startFinishWall[2]; //place the walls nearest to the startline

	for (int i = 0; i < 2; i++)
	{
		startFinishWall[i].wallSeg = wallSegMesh->CreateModel(startFinishWall[i].xPos, startFinishWall[i].yPos, startFinishWall[i].zPos);
		startFinishWall[i].wallSeg->SetX(startFinishWall[i].xPos + (i * 20));
	}

	/*-Far left, initial straight wall-*/

	Wall firstLeftStraight[41]; //the small wall that leads to the long first straight wall
	for (int i = 0; i < 41; i++) //first straight wall 
	{
		firstLeftStraight[i].wallSeg = wallSegMesh->CreateModel();
		firstLeftStraight[i].wallSeg->SetPosition(-31, 0, 4 + (i * 9));
	}


	Isles farLeft[9];//the leftmost isles of the wide outside wall, with one wall segment alongside it
	Wall gateSeg[2];

	for (int i = 0; i < 9; i++)
	{
		if (i < 2)
		{
			gateSeg[i].wallSeg = wallSegMesh->CreateModel(-15 - (9 * i), 0, 53);
			gateSeg[i].wallSeg->RotateY(90); //replace these with matricies?
		}

		if (i > 1)
		{
			farLeft[i].IsleModel = wallendMesh->CreateModel(-31, 0, 0 + ((i - 2) * 53)); //the one spare wall segment

		}
	}

	/*-Back wall-*/

	Wall backwall[34];
	//place and rotate back wall segments
	for (int i = 0; i < 34; i++)
	{
		backwall[i].wallSeg = wallSegMesh->CreateModel();
		backwall[i].wallSeg->RotateY(90); 
		backwall[i].wallSeg->SetPosition((-31 + 5.0f) + (i * 9), 0, 371.0f);
	}

	Isles backIsles[6];
	for (int i = 0; i < 6; i++)
	{
		backIsles[i].IsleModel = wallendMesh->CreateModel();
		backIsles[i].IsleModel->RotateY(90); 
		backIsles[i].IsleModel->SetPosition(-31 + (i * 62), 0, 371.0f);
	}

	/*-Middle Bit-*/
	Wall middleLeftWall[35];
	Wall rightGateSeg[2];
	for (int i = 0; i < 35; i++)
	{
		if (i < 2)
		{
			rightGateSeg[i].wallSeg = wallSegMesh->CreateModel();
			rightGateSeg[i].wallSeg->RotateY(90);
			rightGateSeg[i].wallSeg->SetPosition(15 + (i * 9), 0, 53);
		}
		middleLeftWall[i].wallSeg = wallSegMesh->CreateModel();
		middleLeftWall[i].wallSeg->SetPosition(31, 0, 5.0f + (i * 9));
	}

	Isles middleLeftIsles[7];
	for (int i = 0; i < 7; i++)
	{
		middleLeftIsles[i].IsleModel = wallendMesh->CreateModel();
		middleLeftIsles[i].IsleModel->SetPosition(31, 0, 318 - (i * 53));
	}

	/*-Middle Right Wall, leading into wider area-*/
	Wall MiddleRightWall[36];
	for (int i = 0; i < 36; i++)
	{
		MiddleRightWall[i].wallSeg = wallSegMesh->CreateModel();
		MiddleRightWall[i].wallSeg->SetPosition(93, 0, 371 - (i * 9));
	}
	//isles
	Isles midRightIsles[6];
	midRightIsles->zPos = 318;
	for (int i = 0; i < 6; i++)
	{
		midRightIsles[i].IsleModel = wallendMesh->CreateModel();
		midRightIsles[i].IsleModel->SetPosition(93, 0, midRightIsles->zPos - (i * 53));
	}

	/*-middle bottom wall, part of the final straight-*/
	Wall midBottom[21];
	for (int i = 0; i < 21; i++)
	{
		midBottom[i].wallSeg = wallSegMesh->CreateModel();
		midBottom[i].wallSeg->RotateY(90);
		midBottom[i].wallSeg->SetPosition(32 + 2 + (i * 9), 0, 0);
	}

	//isles
	Isles midBottomIsles[3];
	for (int i = 0; i < 3; i++)
	{
		midBottomIsles[i].IsleModel = wallendMesh->CreateModel();
		midBottomIsles[i].IsleModel->RotateY(90);
		midBottomIsles[i].IsleModel->SetPosition(94 + (i * 62), 0, 0);
	}


	/*-middle set piece-*/
	Isles setPiece[4];
	Wall vertSetPiece[34];
	Wall horizSetPiece[8];
	vertSetPiece->xPos = 139;
	horizSetPiece->zPos = 265;
	for (int i = 0; i < 4; i++)
	{
		setPiece[i].IsleModel = wallendMesh->CreateModel(172, 0, 265);
		if (i < 2)
		{
			setPiece[i].IsleModel->SetPosition(139 + (i * 36), 0, 265);
		}

		if (i > 1)
		{
			setPiece[i].IsleModel->SetPosition(139 + ((i - 2) * 36), 0, 265 - 150);
		}
	}
	//vertical walls of the setpiece
	for (int i = 0; i < 2; i++)
	{
		if (i < 1)
		{
			for (int i = 0; i < 17; i++)
			{
				vertSetPiece[i].wallSeg = wallSegMesh->CreateModel();
				vertSetPiece[i].wallSeg->SetPosition(vertSetPiece->xPos, 0, 119 + (i * 9));
			}
		}

		vertSetPiece->xPos = 175;
		if (i > 0)
		{
			for (int i = 17; i < 34; i++)
			{
				vertSetPiece[i].wallSeg = wallSegMesh->CreateModel();
				vertSetPiece[i].wallSeg->SetPosition(vertSetPiece->xPos, 0, 119 + ((i - 17) * 9));
			}
		}
	}

	//horizontal walls of the set piece
	//must be placed differently due to different sizes
	for (int i = 0; i < 2; i++)
	{
		if (i < 1)
		{
			for (int i = 0; i < 4; i++)
			{
				horizSetPiece[i].wallSeg = wallSegMesh->CreateModel();
				horizSetPiece[i].wallSeg->RotateY(90);
				horizSetPiece[i].wallSeg->SetPosition(143 + (i * 9), 0, horizSetPiece->zPos);
			}
		}
		horizSetPiece->zPos = 115;
		if (i > 0)
		{
			for (int i = 4; i < 8; i++)
			{
				horizSetPiece[i].wallSeg = wallSegMesh->CreateModel();
				horizSetPiece[i].wallSeg->RotateY(90);
				horizSetPiece[i].wallSeg->SetPosition(143 + ((i - 4) * 9), 0, horizSetPiece->zPos);
			}
		}

	}

	/*-Right hand middle wall-*/
	Isles finalRightIsles[7];
	Wall finalRightWall[36];

	finalRightWall->xPos = 218;

	for (int i = 0; i < 36; i++)
	{
		finalRightWall[i].wallSeg = wallSegMesh->CreateModel();
		finalRightWall[i].wallSeg->SetPosition(finalRightWall->xPos, 0, 0 + (i * 9));
	}

	for (int i = 0; i < 7; i++)
	{
		finalRightIsles[i].IsleModel = wallendMesh->CreateModel();
		finalRightIsles[i].IsleModel->SetPosition(218, 0, i * 53);
	}

	/*-Far Right Wall-*/
	Isles farRightIsles[9];
	farRightIsles->zPos = 371.0f - 53.0f;
	Wall farRightWalls[53];
	farRightWalls->zPos = 371.0f;
	for (int i = 0; i < 9; i++)
	{
		farRightIsles[i].IsleModel = wallendMesh->CreateModel();
		farRightIsles[i].IsleModel->SetPosition(279, 0, farRightIsles->zPos - (i * 53));
	}

	for (int i = 0; i < 53; i++)
	{
		farRightWalls[i].wallSeg = wallSegMesh->CreateModel();
		farRightWalls[i].wallSeg->SetPosition(279, 0, (farRightWalls->zPos - farRightWalls->wallOfset) - (i * 9));
	}

	/*-Back Straight set piece-*/
	Isles backStraightPieceIsles[30];
	backStraightPieceIsles->xPos = 240;
	backStraightPieceIsles->zPos = 218 - 6.0f;

	Wall  backStraightPieceWalls[35];
	backStraightPieceWalls->xPos = 240;
	backStraightPieceWalls->zPos = 212 - backStraightPieceWalls->wallOfset;
	Wall  backStraightGateSeg[4];

	//the two isles that are part of the gate
	for (int i = 0; i < 2; i++)
	{
		backStraightPieceIsles[i].IsleModel = wallendMesh->CreateModel();
		backStraightPieceIsles[i].IsleModel->RotateY(90);
		backStraightPieceIsles[i].IsleModel->SetPosition(backStraightPieceIsles->xPos + (i * 18), 0, backStraightPieceIsles->zPos);


	}

	//the series of islands that make up the set piece
	for (int i = 0; i < 2; i++)
	{
		//left hand side
		if (i == 0)
		{
			backStraightPieceIsles->xPos = 240;
			backStraightPieceIsles->zPos = 110;
			for (int i = 2; i < 15; i++)
			{
				//only change the values through statements, don't actually change them each time a block is moved
				if (i > 2 && i < 6)
				{
					backStraightPieceIsles->xPos = 236 - ((i - 3) * 4);
					backStraightPieceIsles->zPos = 104 - ((i - 3) * 6);
				}
				if (i > 5 && i < 9)
				{
					backStraightPieceIsles->zPos = 104 - ((i - 4) * 6);
				}

				if (i > 10)
				{
					backStraightPieceIsles->xPos = 220 + ((i - 9) * 4);
					backStraightPieceIsles->zPos = 86 - ((i - 9) * 6);
				}
				backStraightPieceIsles[i].IsleModel = wallendMesh->CreateModel();
				backStraightPieceIsles[i].IsleModel->SetPosition(backStraightPieceIsles->xPos, 0, backStraightPieceIsles->zPos);
			}
		}

		//right hand side
		if (i == 1)
		{
			backStraightPieceIsles->xPos = 258;
			backStraightPieceIsles->zPos = 110;
			for (int j = 15; j < 30; j++)
			{
				if (j > 15 && j < 19)
				{
					backStraightPieceIsles->xPos = backStraightPieceIsles->xPos + 4;
					backStraightPieceIsles->zPos = backStraightPieceIsles->zPos - 6;
				}

				if (j > 18 && j < 22)
				{
					backStraightPieceIsles->zPos = backStraightPieceIsles->zPos - 6;
				}

				if (j > 21 && j < 25)
				{
					backStraightPieceIsles->xPos = backStraightPieceIsles->xPos - 4;
					backStraightPieceIsles->zPos = backStraightPieceIsles->zPos - 6;
				}
				backStraightPieceIsles[j].IsleModel = wallendMesh->CreateModel();
				backStraightPieceIsles[j].IsleModel->SetPosition(backStraightPieceIsles->xPos, 0, backStraightPieceIsles->zPos);
			}

		}
	}




	//the sideways wall segment that prevents the player from going past the set piece
	for (int i = 0; i < 4; i++)
	{
		backStraightGateSeg[i].wallSeg = wallSegMesh->CreateModel();
		backStraightGateSeg[i].wallSeg->RotateY(90);

		if (i < 2)
		{
			backStraightGateSeg[i].wallSeg->SetPosition((219 + backStraightGateSeg->wallOfset) + (i * 9), 0, 212);
		}

		if (i > 1 && i < 4)
		{
			backStraightGateSeg[i].wallSeg->SetPosition(279 - backStraightGateSeg->wallOfset - ((i - 2) * 9), 0, 212);
		}
	}

	//the two narrow walls between the set piece, including the gap between them

	for (int i = 0; i < 11; i++)
	{
		backStraightPieceWalls[i].wallSeg = wallSegMesh->CreateModel();
		backStraightPieceWalls[i].wallSeg->SetPosition(backStraightPieceWalls->xPos, 0, backStraightPieceWalls->zPos - (i * 9));
	}

	backStraightPieceWalls->xPos = 258;

	for (int i = 11; i < 22; i++)
	{
		backStraightPieceWalls[i].wallSeg = wallSegMesh->CreateModel();
		backStraightPieceWalls[i].wallSeg->SetPosition(backStraightPieceWalls->xPos, 0, backStraightPieceWalls->zPos - ((i - 11) * 9));
	}


	//right wall
	backStraightPieceWalls->zPos = 50;
	for (int i = 22; i < 28; i++)
	{
		backStraightPieceWalls[i].wallSeg = wallSegMesh->CreateModel();
		backStraightPieceWalls[i].wallSeg->SetPosition(backStraightPieceWalls->xPos, 0, backStraightPieceWalls->zPos - ((i - 22) * 9));
	}

	backStraightPieceWalls->xPos = 240;

	for (int i = 28; i < 35; i++)
	{
		backStraightPieceWalls[i].wallSeg = wallSegMesh->CreateModel();
		backStraightPieceWalls[i].wallSeg->SetPosition(backStraightPieceWalls->xPos, 0, backStraightPieceWalls->zPos - ((i - 29) * 9));
	}


	/*-bottom wall*/

	Isles bottomIsles[6];

	Wall  bottomWall[34];

	Wall  finalStraightObstacles[21];
	finalStraightObstacles->xPos = 154;
	finalStraightObstacles->zPos = -106.0f + finalStraightObstacles->wallOfset; //inline with 3rd right isle on the bottom row

	Wall smallLeftCorner[14];
	smallLeftCorner->xPos = -31;
	smallLeftCorner->zPos = -106 + smallLeftCorner->wallOfset;
	//same length as the top wall
	for (int i = 0; i < 34; i++)
	{
		bottomWall[i].wallSeg = wallSegMesh->CreateModel();
		bottomWall[i].wallSeg->RotateY(90); //matricies?
		bottomWall[i].wallSeg->SetPosition((-31 + 5.0f) + (i * 9), 0, -106.0f);
	}

	for (int i = 0; i < 6; i++)
	{
		bottomIsles[i].IsleModel = wallendMesh->CreateModel();
		bottomIsles[i].IsleModel->RotateY(90); //matricies?
		bottomIsles[i].IsleModel->SetPosition(-31 + (i * 62), 0, -106.0f);
	}

	/*Bottom wall obstacle set piece*/
	for (int i = 0; i < 3; i++)
	{
		if (i == 0)
			for (int i = 0; i < 7; i++)
			{
				finalStraightObstacles[i].wallSeg = wallSegMesh->CreateModel();
				finalStraightObstacles[i].wallSeg->SetPosition(finalStraightObstacles->xPos, 0, finalStraightObstacles->zPos);
				finalStraightObstacles->zPos += 9;
			}

		if (i == 1)
		{
			finalStraightObstacles->xPos = 93;
			finalStraightObstacles->zPos = 0 - finalStraightObstacles->wallOfset;
			for (int i = 7; i < 14; i++)
			{
				finalStraightObstacles[i].wallSeg = wallSegMesh->CreateModel();
				finalStraightObstacles[i].wallSeg->SetPosition(finalStraightObstacles->xPos, 0, finalStraightObstacles->zPos);
				finalStraightObstacles->zPos -= 9;
			}
		}

		if (i == 2)
		{
			finalStraightObstacles->xPos = 31;
			finalStraightObstacles->zPos = -106.0f + finalStraightObstacles->wallOfset;
			for (int i = 14; i < 21; i++)
			{
				finalStraightObstacles[i].wallSeg = wallSegMesh->CreateModel();
				finalStraightObstacles[i].wallSeg->SetPosition(finalStraightObstacles->xPos, 0, finalStraightObstacles->zPos);
				finalStraightObstacles->zPos += 9;
			}
		}
	}

	/*final bottom part of the left wall to make up the last corner*/
	for (int i = 0; i < 14; i++)
	{
		smallLeftCorner[i].wallSeg = wallSegMesh->CreateModel();
		smallLeftCorner[i].wallSeg->SetPosition(smallLeftCorner->xPos, 0, smallLeftCorner->zPos + (i * 9));
	}


	/*----Create Water Tanks----*/
	IMesh * tankMesh = myEngine->LoadMesh("TankSmall1.x");

	IModel * Tanks[6];
	float tankPosX[6]{ 30,  93,  216, 62, 155, 250 };
	float tankPosZ[6]{ 328, 40,  330, 290, 100, 85 };

	for (int i = 0; i < 6; i++)
	{
		Tanks[i] = tankMesh->CreateModel(tankPosX[i], 0, tankPosZ[i]);
		if (i > 2)
		{
			Tanks[i]->RotateX(-45);
		}
	}

	/*----create checkpoints----*/
	IMesh * checkpointMesh = myEngine->LoadMesh("Checkpoint.x");

	//use for every checkpoint to track position, use numLegs as a constant
	struct Checkpoint
	{
		IModel * checkModel;
		float xPos = 0;
		float yPos = 0;
		float zPos = 0;
		const int numLegs = 2;
	};

	//splitting the amount of checkpoints in half 
	Checkpoint checkPoints[4];
	for (int i = 0; i < 4; i++)
	{
		checkPoints[i].checkModel = checkpointMesh->CreateModel(checkPoints[i].xPos, checkPoints[i].yPos, checkPoints[i].zPos);

		if (i > 0 && i < 2)
			checkPoints[i].checkModel->SetZ(checkPoints[i].zPos + (i * 100));

		if (i > 1 && i < 3)
		{
			checkPoints[i].checkModel->SetPosition(checkPoints[i].xPos + 249, checkPoints[i].yPos, checkPoints[i].zPos);
		}

		if (i > 2)
		{
			checkPoints[i].checkModel->SetPosition(checkPoints[i].xPos + 62, checkPoints[i].yPos, checkPoints[i].zPos + 100);
		}
	}

	Checkpoint otherCheckPoints[3]; //checkpoints 3, 5, and 6
	float otherCheckPointsXPos[3]{ 62, 250, 101 };
	float otherCheckPointsZPos[3]{ 335, 320, 20 };

	for (int i = 0; i < 3; i++)
	{
		otherCheckPoints[i].checkModel = checkpointMesh->CreateModel(otherCheckPointsXPos[i], 0, otherCheckPointsZPos[i]);
		if (i > 1)
		{
			otherCheckPoints[i].checkModel->RotateY(90);
		}
	}

	/*----create a camera----*/
	struct Camera
	{
		ICamera * myCamera;
		int cameraSpeed = 20;
		float cameraRotateAngleX = 10.0f;
		float cameraRotateAngleY = 0.0f;

	};

	Camera camera;
	camera.myCamera = myEngine->CreateCamera(kManual, 0, 0, -30);
	camera.myCamera->AttachToParent(playerCar.model);
	camera.myCamera->RotateX(10);
	camera.myCamera->SetLocalY(15);
	camera.myCamera->SetLocalZ(-35);



	float frameTime;
	myEngine->Timer();

	float countdownTimer = 0;
	float boostTimer = 0;
	float cooldownTimer = 0;
	// The main game loop, repeat until engine is stopped
	while (myEngine->IsRunning())
	{
		// Draw the scene
		myEngine->DrawScene();
		frameTime = myEngine->Timer();

		float xPos = playerCar.model->GetX();//store positions before anything is done
		float zPos = playerCar.model->GetZ();

		/**** Update your scene each frame here ****/
		
		//before game start
		if (playerCar.state == WAITING)
		{
			stringstream startText;
			startText << "Press Spacebar to Start";
			uiFont->Draw(startText.str(), 680, 680);

			if (myEngine->KeyHit(Key_Space))
			{
				playerCar.state = COUNTDOWN;
			}
		}

		//countdown state
		if (playerCar.state == COUNTDOWN)
		{
			countdownTimer += frameTime;

			if (countdownTimer <= 1 && countdownTimer > 0) //draw text between two numbers
			{
				stringstream start3Text;
				start3Text << "3";
				raceFont->Draw(start3Text.str(), 640, 240, kBlack, kCentre, kVCentre);
			}
			if (countdownTimer <= 2 && countdownTimer > 1)
			{
				stringstream start2Text;
				start2Text << "2";
				raceFont->Draw(start2Text.str(), 640, 240, kBlack, kCentre, kVCentre);
			}
			if (countdownTimer <= 3 && countdownTimer > 2)
			{
				stringstream start1Text;
				start1Text << "1";
				raceFont->Draw(start1Text.str(), 640, 240, kBlack, kCentre, kVCentre);
			}

			if (countdownTimer >= 3)
			{
				playerCar.state = CHECK1;
				cpuCar.state = CHECK1;
			}

			if (myEngine->KeyHit(quitKey))
			{
				myEngine->Stop();
			}
		}

		//Second Checkpoint
		if (playerCar.state == CHECK1 && PointToBoxCollision(xPos, zPos, 0.0f, 0.0f, 7.0f, 0.5) && playerCar.lap < 4)
		{
			playerCar.state = CHECK2;
			playerCar.lap++;
		}

		//Third Checkpoint
		if (playerCar.state == CHECK2 && PointToBoxCollision(xPos, zPos, 0.0f, 100.0f, 7.0f, 0.5))
		{
			playerCar.state = CHECK3;
		}

		//Fourth Checkpoint
		if (playerCar.state == CHECK3 && PointToBoxCollision(xPos, zPos, otherCheckPointsXPos[0], otherCheckPoints[0].checkModel->GetZ(), 7.0f, 0.5))
		{
			playerCar.state = CHECK4;
		}

		//Fifth Checkpoint
		if (playerCar.state == CHECK4 && PointToBoxCollision(xPos, zPos, 62, 100, 7.0f, 0.5))
		{
			playerCar.state = CHECK5;
		}

		//Sixth Checkpoint
		if (playerCar.state == CHECK5 && PointToBoxCollision(xPos, zPos, otherCheckPointsXPos[2], otherCheckPoints[2].checkModel->GetZ(), 0.5f, 7.0f))
		{
			playerCar.state = CHECK6;
		}

		//Seventh Checkpoint
		if (playerCar.state == CHECK6 && PointToBoxCollision(xPos, zPos, otherCheckPointsXPos[1], otherCheckPoints[1].checkModel->GetZ(), 7.0f, 0.5f))
		{
			playerCar.state = CHECK7;
		}

		//New lap
		if (playerCar.state == CHECK7 && PointToBoxCollision(xPos, zPos, 249.0f, 0.0f, 7.0f, 0.5f))
		{
			playerCar.state = CHECK1;
		}

		//after three laps, end the game
		if (playerCar.state == CHECK1 && PointToBoxCollision(xPos, zPos, 0.0f, 0.0f, 7.0f, 0.5) && playerCar.lap > 3)
		{
			playerCar.state = FINISH;
		}


		if (playerCar.state == FINISH)
		{

		}

		//
		if (playerCar.state == CHECK1 || playerCar.state == CHECK2 || playerCar.state == CHECK3 || playerCar.state == CHECK4 || playerCar.state == CHECK5 || playerCar.state == CHECK6 ||
			playerCar.state == CHECK7)
		{
			//get the facing vector
			moveDummy->GetMatrix(&moveDummyMatrix[0][0]);
			vector2D localZVec = { moveDummyMatrix[2][0], moveDummyMatrix[2][2] };

			//ai car directions
			if (PointToBoxCollision(cpuCar.model->GetX(), cpuCar.model->GetZ(), waypoint[currentWayPoint]->GetX(), waypoint[currentWayPoint]->GetZ(), 1, 1))
			{
				//add to the current waypoint while we haven't reached the last one
				if (currentWayPoint < 27)
				{
					currentWayPoint++;
				}

				//loop back to first waypoint once all previous ones have been passed, do so for all laps
				if (currentWayPoint >= 26)
				{
					currentWayPoint = 0;
				}
			}

			if (currentWayPoint < 27)
			{
				cpuCar.model->LookAt(waypoint[currentWayPoint]);
			}

			//calculate forces on key press
			if (myEngine->KeyHeld(accelerateKey))
			{
				thrust = scalar(playerCar.thrustFactor, localZVec);
			}

			else if (myEngine->KeyHeld(brakeKey))
			{
				thrust = scalar(-playerCar.reverseThrust, localZVec);
			}
			else
			{
				thrust = { 0.0f, 0.0f };
			}
			//calculate drag (based on previous momentum
			drag = scalar(carDragCoeff, momentum);

			//calculate momentum (based on thrust, drag and previous momentum

			momentum = sumMovVectors(momentum, thrust, drag);

			//actually move the car

			//but first calclate the new positions
			float newXPos = xPos + momentum.x * frameTime;
			float newZPos = zPos + momentum.z * frameTime;

			/*----COLLISIONS----*/

			//initial two walls
			for (int i = 0; i < 2; i++)
			{
				collisionSide collisionResponse = SphereToBoxCollision2D(xPos, zPos, newXPos, newZPos, 2.5, startFinishWall[i].wallSeg->GetX(), startFinishWall[i].wallSeg->GetZ(), 2.0, 10.0);
				CollisionResolution(collisionResponse, momentum);
			}

			//far left wall
			collisionSide farLeftCollision = SphereToBoxCollision2D(xPos, zPos, newXPos, newZPos, 2.5, firstLeftStraight[14].wallSeg->GetX(), firstLeftStraight[14].wallSeg->GetZ(), 2.0f, 240.5f);
			CollisionResolution(farLeftCollision, momentum);

			//back wall 
			collisionSide backWallCollision = SphereToBoxCollision2D(xPos, zPos, newXPos, newZPos, 2.5, backwall[17].wallSeg->GetX(), backwall[17].wallSeg->GetZ(), 157.7f, 2.0f); //322 units long
			CollisionResolution(backWallCollision, momentum);


			//middle left walls
			collisionSide middleLeftCollision = SphereToBoxCollision2D(xPos, zPos, newXPos, newZPos, 2.5, middleLeftWall[17].wallSeg->GetX(), middleLeftWall[17].wallSeg->GetZ(), 2.0f, 163.0f);
			CollisionResolution(middleLeftCollision, momentum);

			//middle-middle wall
			collisionSide middleMidCollision = SphereToBoxCollision2D(xPos, zPos, newXPos, newZPos, 2.5, MiddleRightWall[17].wallSeg->GetX(), MiddleRightWall[17].wallSeg->GetZ(), 2.0f, 160.0f);
			CollisionResolution(middleMidCollision, momentum);

			//midBottomWall
			collisionSide bottomMidCollision = SphereToBoxCollision2D(xPos, zPos, newXPos, newZPos, 2.5, midBottom[10].wallSeg->GetX(), midBottom[10].wallSeg->GetZ(), 95.0f, 2.0f);
			CollisionResolution(bottomMidCollision, momentum);

			//middle set piece left vertical wall
			collisionSide setPieceVertLeftColl = SphereToBoxCollision2D(xPos, zPos, newXPos, newZPos, 2.5, vertSetPiece[8].wallSeg->GetX(), vertSetPiece[8].wallSeg->GetZ(), 2.0f, 75.5f);
			CollisionResolution(setPieceVertLeftColl, momentum);

			//middle set piece right vertical wall
			collisionSide setPieceVertRightColl = SphereToBoxCollision2D(xPos, zPos, newXPos, newZPos, 2.5, vertSetPiece[25].wallSeg->GetX(), vertSetPiece[25].wallSeg->GetZ(), 2.0f, 75.5f);
			CollisionResolution(setPieceVertRightColl, momentum);

			//middle set piece top horizontal wall
			collisionSide setPieceHorizTopColl = SphereToBoxCollision2D(xPos, zPos, newXPos, newZPos, 2.5, horizSetPiece[1].wallSeg->GetX() + 5.0f, horizSetPiece[1].wallSeg->GetZ(), 18.0f, 2.0f);
			CollisionResolution(setPieceHorizTopColl, momentum);

			//middle set piece bottom horizontal wall
			collisionSide setPieceHorizBotColl = SphereToBoxCollision2D(xPos, zPos, newXPos, newZPos, 2.5, horizSetPiece[5].wallSeg->GetX() + 5.0f, horizSetPiece[5].wallSeg->GetZ(), 18.0f, 2.0f);
			CollisionResolution(setPieceHorizBotColl, momentum);

			//middle right wall
			collisionSide middleRightCollision = SphereToBoxCollision2D(xPos, zPos, newXPos, newZPos, 2.5, finalRightWall[17].wallSeg->GetX(), finalRightWall[17].wallSeg->GetZ(), 2.0f, 161.0f);
			CollisionResolution(middleRightCollision, momentum);

			//far right wall
			collisionSide farRightCollision = SphereToBoxCollision2D(xPos, zPos, newXPos, newZPos, 2.5, farRightWalls[25].wallSeg->GetX() + 5.0f, farRightWalls[25].wallSeg->GetZ(), 2.0f, 238.5f);
			CollisionResolution(farRightCollision, momentum);

			//gate for the far right set piece, left hand side
			collisionSide setPieceGateLeftCollision = SphereToBoxCollision2D(xPos, zPos, newXPos, newZPos, 2.5, backStraightGateSeg[0].wallSeg->GetX() + 5, backStraightGateSeg[0].wallSeg->GetZ(), 10.0f, 2.0f);
			CollisionResolution(setPieceGateLeftCollision, momentum);

			//gate for the far right set piece, right hand side
			collisionSide setPieceGateRightCollision = SphereToBoxCollision2D(xPos, zPos, newXPos, newZPos, 2.5, backStraightGateSeg[2].wallSeg->GetX() - 5, backStraightGateSeg[2].wallSeg->GetZ(), 10.0f, 2.0f);
			CollisionResolution(setPieceGateRightCollision, momentum);

			//left wall straight part of the set piece
			collisionSide setPieceWallLeftCollision = SphereToBoxCollision2D(xPos, zPos, newXPos, newZPos, 2.5, backStraightPieceWalls[5].wallSeg->GetX(), backStraightPieceWalls[5].wallSeg->GetZ(), 2.0f, 50.0f);
			CollisionResolution(setPieceWallLeftCollision, momentum);

			//right wall straight part of the set piece
			collisionSide setPieceWallRightCollision = SphereToBoxCollision2D(xPos, zPos, newXPos, newZPos, 2.5, backStraightPieceWalls[16].wallSeg->GetX(), backStraightPieceWalls[16].wallSeg->GetZ(), 2.0f, 50.0f);
			CollisionResolution(setPieceWallRightCollision, momentum);

			//Second straight wall that's part of the set peice 
			collisionSide setPieceBottomLeftColl = SphereToBoxCollision2D(xPos, zPos, newXPos, newZPos, 2.5, backStraightPieceWalls[25].wallSeg->GetX(), backStraightPieceWalls[25].wallSeg->GetZ(), 2.0f, 27.0);
			CollisionResolution(setPieceBottomLeftColl, momentum);

			//second straigh wall that is also part of the set piece
			collisionSide setPieceBottomRightColl = SphereToBoxCollision2D(xPos, zPos, newXPos, newZPos, 3.0, backStraightPieceWalls[31].wallSeg->GetX(), backStraightPieceWalls[31].wallSeg->GetZ(), 2.0f, 27.0);
			CollisionResolution(setPieceBottomRightColl, momentum);

			//bottom wall - final straight
			collisionSide bottomWallCollision = SphereToBoxCollision2D(xPos, zPos, newXPos, newZPos, 2.5, bottomWall[17].wallSeg->GetX(), bottomWall[17].wallSeg->GetZ(), 157.7f, 2.0f); //322 units long
			CollisionResolution(bottomWallCollision, momentum);

			//Three gimmick walls
			collisionSide finalStraightObstacle1Collision = SphereToBoxCollision2D(xPos, zPos, newXPos, newZPos, 2.5, finalStraightObstacles[3].wallSeg->GetX(), finalStraightObstacles[3].wallSeg->GetZ(), 2.0f, 31.5f);
			CollisionResolution(finalStraightObstacle1Collision, momentum);


			collisionSide finalStraightObstacle2Collision = SphereToBoxCollision2D(xPos, zPos, newXPos, newZPos, 2.5, finalStraightObstacles[10].wallSeg->GetX(), finalStraightObstacles[10].wallSeg->GetZ(), 2.0f, 31.5f);
			CollisionResolution(finalStraightObstacle2Collision, momentum);

			collisionSide finalStraightObstacle3Collision = SphereToBoxCollision2D(xPos, zPos, newXPos, newZPos, 2.5, finalStraightObstacles[17].wallSeg->GetX(), finalStraightObstacles[17].wallSeg->GetZ(), 2.0f, 31.5f);
			CollisionResolution(finalStraightObstacle3Collision, momentum);

			/*Sphere collisions*/

			for (int i = 0; i < checkPoints->numLegs; i++) //4 checks, number of legs * 2
			{
				//collision for the legs of checkpoints 1, 2, and 7
				if (i < checkPoints->numLegs)
				{
					bool startFinishCPCollision = SphereToSphereCollision2D(xPos, zPos, 2.5f, -9.0f + (i * 18), 0.0f, 1.0f);
					if (startFinishCPCollision == true)
					{
						momentum.z = -momentum.z;
						momentum.x = -momentum.x;
					}

					bool CP2Collision = SphereToSphereCollision2D(xPos, zPos, 2.5f, -9.0f + (i * 18), 100.0f, 1.0f);
					if (CP2Collision == true)
					{
						momentum.z = -momentum.z;
						momentum.x = -momentum.x;
					}

				}

				if (i > 1)
				{
					bool CP8Collision = SphereToSphereCollision2D(xPos, zPos, 2.5f, 258 - ((i - checkPoints->numLegs)) * 18, 0.0f, 1.0f);
					{
						if (CP8Collision == true)
						{
							momentum.z = -momentum.z;
							momentum.x = -momentum.x;
						}
					}

				}
			}
			//collision for the legs of checkpoint 3
			for (int i = 0; i < 2; i++)
			{
				bool cp3Collision = SphereToSphereCollision2D(xPos, zPos, 2.5f, otherCheckPoints[0].checkModel->GetX() - 9 + (i * 18), otherCheckPoints[0].checkModel->GetZ(), 1.0f);
				if (cp3Collision == true)
				{
					momentum.z = -momentum.z;
					momentum.x = -momentum.x;
				}
			}
			//collision for the legs of checkpoint 4
			for (int i = 0; i < 2; i++)
			{
				bool cp4Collision = SphereToSphereCollision2D(xPos, zPos, 2.5f, checkPoints[3].checkModel->GetX() - 9 + (i * 18), checkPoints[3].checkModel->GetZ(), 1.0f);
				if (cp4Collision == true)
				{
					momentum.z = -momentum.z;
					momentum.x = -momentum.x;
				}
			}

			//collision for the legs of checkpoint 5
			for (int i = 0; i < 2; i++)
			{
				bool cp5Collision = SphereToSphereCollision2D(xPos, zPos, 2.5f, otherCheckPoints[2].checkModel->GetX(), otherCheckPoints[2].checkModel->GetZ() - 9 + (i * 18), 1.0f);
				if (cp5Collision == true)
				{
					momentum.z = -momentum.z;
					momentum.x = -momentum.x;
				}
			}

			//collision for the legs of checkpoint 6
			for (int i = 0; i < 2; i++)
			{
				bool cp6Collision = SphereToSphereCollision2D(xPos, zPos, 2.5f, otherCheckPoints[1].checkModel->GetX() - 9 + (i * 18), otherCheckPoints[1].checkModel->GetZ(), 1.0f);
				if (cp6Collision == true)
				{
					momentum.z = -momentum.z;
					momentum.x = -momentum.x;
				}
			}

			//collision for the water tanks
			for (int i = 0; i < 6; i++)
			{
				bool tankCollision = SphereToSphereCollision2D(xPos, zPos, 2.0f, Tanks[i]->GetX(), Tanks[i]->GetZ(), 3.0f);
				if (tankCollision == true)
				{
					momentum.z = -momentum.z;
					momentum.x = -momentum.x;
				}
			}

			//left side of the far right set piece
			for (int i = 0; i < 30; i++)
			{
				bool farRightSetPieceLeftColl[30];
				farRightSetPieceLeftColl[i] = SphereToSphereCollision2D(xPos, zPos, 2.5, backStraightPieceIsles[i].IsleModel->GetX(), backStraightPieceIsles[i].IsleModel->GetZ(), 3.5f);

				if (farRightSetPieceLeftColl[i] == true)
				{
					momentum.z = -momentum.z;
					momentum.x = -momentum.x;
				}
			}

			//Check car collision
			bool carToCarCollision = SphereToSphereCollision2D(xPos, zPos, 2.0f, cpuCar.model->GetX(), cpuCar.model->GetZ(), 2.0f);
			if (carToCarCollision == true)
			{
				momentum.z = -momentum.z;
				momentum.x = -momentum.x;
			}

			//move the car's animation dummy, using matricies
			moveDummyMatrix[3][0] += (momentum.x * frameTime);
			moveDummyMatrix[3][2] += (momentum.z * frameTime);
			moveDummy->SetMatrix(&moveDummyMatrix[0][0]);

			//make the cpu car move
			cpuCar.model->MoveLocalZ(cpuCar.currentSpeed * frameTime);

			//display relevant race text
			playerCar.currentSpeed = sqrt(pow(momentum.x * frameTime, 2) + pow(momentum.z * frameTime, 2)) * 1000;
			stringstream speedText;
			speedText << "Speed: " << playerCar.currentSpeed;
			uiFont->Draw(speedText.str(), 0, 680);

			stringstream stateTest;
			stateTest << "| Stage: " << playerCar.state;
			uiFont->Draw(stateTest.str(), 200, 680);

			stringstream lap;
			lap << "| Lap:" << playerCar.lap;
			uiFont->Draw(lap.str(), 400, 680);

			//animate the car
			if (currentUpHover >= maxUpHover)
			{
				isTop = true;
			}
			else if (currentUpHover <= 0.0f)
			{
				isTop = false;
			}

			if (isTop == true)
			{
				hoverDummy->MoveY(-HoverIncrement * frameTime);
				currentUpHover -= HoverIncrement * frameTime;
				camera.myCamera->MoveY(+HoverIncrement * frameTime);
			}

			if (isTop == false)
			{
				hoverDummy->MoveY(+HoverIncrement * frameTime);
				currentUpHover += HoverIncrement * frameTime;
				camera.myCamera->MoveY(-HoverIncrement * frameTime);
			}
			//calculate whether we want thrust
			if (myEngine->KeyHeld(leftTurnKey))
			{
				moveDummy->RotateLocalY(-80.0f * frameTime);
			}

			if (myEngine->KeyHeld(rightTurnKey))
			{
				moveDummy->RotateLocalY(80.0f * frameTime);
			}

			//Set flags for boosting and overheating, display to player
			if (myEngine->KeyHeld(Key_Space) && isOverheated == false)
			{
				playerCar.thrustFactor = 0.1;
				boostTimer += frameTime;

				if (boostTimer > 0 && boostTimer <= 2)
				{
					stringstream boostBoys;
					boostBoys << "Boosting";
					uiFont->Draw(boostBoys.str(), 600, 680);
				}


				if (boostTimer > 2 && boostTimer <= 3)
				{
					stringstream boostText;
					boostText << "OverHeating!!";
					uiFont->Draw(boostText.str(), 600, 680);
				}

				//overheated
				if (boostTimer > 3)
				{
					playerCar.thrustFactor = 0.001;
					isOverheated = true;

				}
			}

			//make the car boost
			if (!myEngine->KeyHeld(Key_Space))
			{
				playerCar.thrustFactor = 0.05;
				boostTimer = 0;
			}

			if (isOverheated == true)
			{

				cooldownTimer += frameTime;
				stringstream ohText;
				ohText << "OverHeated!!";
				uiFont->Draw(ohText.str(), 600, 680);

				if (cooldownTimer >= 5)
				{

					boostTimer = 0;
					cooldownTimer = 0;
					isOverheated = false;

				}
			}

			//camera controls
			if (myEngine->KeyHeld(Key_Up))
			{
				camera.myCamera->MoveLocalZ(frameTime * camera.cameraSpeed);
			}

			if (myEngine->KeyHeld(Key_Down))
			{
				camera.myCamera->MoveLocalZ(-frameTime * camera.cameraSpeed);
			}

			if (myEngine->KeyHeld(Key_Left))
			{
				camera.myCamera->MoveLocalX(-frameTime * camera.cameraSpeed);
			}

			if (myEngine->KeyHeld(Key_Right))
			{
				camera.myCamera->MoveLocalX(frameTime * camera.cameraSpeed);
			}

			//reset camera orientation
			if (myEngine->KeyHit(Key_1))
			{
				camera.myCamera->ResetOrientation();
				camera.cameraRotateAngleX = 10.0f;
				camera.cameraRotateAngleY = 0.0f;
				camera.myCamera->SetX(xPos);
				camera.myCamera->SetY(0);
				camera.myCamera->SetZ(zPos);
				camera.myCamera->MoveLocal(0, 15, -35);
			}

			//first person camera
			if (myEngine->KeyHit(Key_2))
			{
				camera.myCamera->ResetOrientation();
				camera.cameraRotateAngleX = 0.0f;
				camera.cameraRotateAngleY = 0.0f;
				camera.myCamera->SetX(xPos);
				camera.myCamera->SetY(0);
				camera.myCamera->SetZ(zPos);
				camera.myCamera->MoveLocal(0, 4, 5);
			}

			//prevent camera from flipping upside-down
			camera.cameraRotateAngleX += myEngine->GetMouseMovementY();
			camera.cameraRotateAngleY += myEngine->GetMouseMovementX();

			if (camera.cameraRotateAngleX > 90.0f)
			{
				camera.cameraRotateAngleX = 90.0f;
			}

			if (camera.cameraRotateAngleX < -90.0f)
			{
				camera.cameraRotateAngleX = -90.0f;
			}

			camera.myCamera->ResetOrientation();
			camera.myCamera->RotateLocalY(camera.cameraRotateAngleY);
			camera.myCamera->RotateLocalX(camera.cameraRotateAngleX);


			if (myEngine->KeyHit(quitKey))
			{
				myEngine->Stop();
			}
		}
	}

	// Delete the 3D engine now we are finished with it
	myEngine->Delete();
}

collisionSide SphereToBoxCollision2D(float sphereOldX, float sphereOldZ, float sphereX, float sphereZ, float sphereRadius,
	float boxXPos, float boxZPos, float boxHorizRadius, float boxVertRadius)
{
	//setup bounding box
	float maxX = boxXPos + boxHorizRadius + sphereRadius;
	float minX = boxXPos - boxHorizRadius - sphereRadius;
	float maxZ = boxZPos + boxVertRadius + sphereRadius;
	float minZ = boxZPos - boxVertRadius - sphereRadius;

	//check if centre of sphere is within bounding box
	if (sphereX > minX && sphereX < maxX && sphereZ > minZ && sphereZ < maxZ)
	{
		//collision - test for axis of collision
		if (sphereOldX < minX || sphereOldX > maxX)
			return Xaxis; //colliding on a line parallel to the x axis
		else if (sphereOldZ < minZ || sphereOldZ > maxZ)
			return zAxis; //colliding on a line parallel to the z axis
	}
	else
		return none;
}

bool SphereToSphereCollision2D(float sphere1XPos, float sphere1ZPos, float sphere1Rad,
	float sphere2XPos, float sphere2ZPos, float sphere2Radius)
{
	float distX = sphere2XPos - sphere1XPos;
	float distZ = sphere2ZPos - sphere1ZPos;

	float distance = sqrt(distX*distX + distZ * distZ);

	return (distance < (sphere1Rad + sphere2Radius));
}

bool PointToBoxCollision(float pointXPos, float pointZPos, float boxXPos, float boxZPos, float boxHorizRadius, float boxVertRadius)
{
	float maxX = boxXPos + boxHorizRadius;
	float minX = boxXPos - boxHorizRadius;
	float maxZ = boxZPos + boxVertRadius;
	float minZ = boxZPos - boxVertRadius;

	if (pointXPos > minX && pointXPos < maxX && pointZPos > minZ && pointZPos < maxZ)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void CollisionResolution(collisionSide collisionResponse, vector2D &momentum)
{
	//resolve
	if (collisionResponse == Xaxis)
	{

		momentum.x = -momentum.x;

	}
	else if (collisionResponse == zAxis)
	{
		momentum.z = -momentum.z;
	}

	//ignore this, messes with collision
	else if (collisionResponse == none)
	{
		//playerCar.model->SetPosition(newXPos, 0.0, newZPos);
	}
}

















