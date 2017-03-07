#include <iostream>
#include <pthread.h>
#include <sys/socket.h>
#include <resolv.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "Matrix.hpp"

// Master matrix dimentions (master matrix is 2d array of matrix objects representing nodes)
#define MASTER_MATRIX_W 4
#define MASTER_MATRIX_H 3
#define AMT_OF_NODES (MASTER_MATRIX_W * MASTER_MATRIX_H)

// Dimentions of 2d char array matrix held on nodes
int matrixW = 8;
int matrixH = matrixW;
int matrixArea = (matrixH * matrixW);

// Flag to signal that the a nodes matrix need to be resized 
bool resizeMatrix = false;

// Mutex for acessing shared data in memory 
pthread_mutex_t rMutex;

/* 
*   Returns 2d character array givin a width and height 
*   array is loaded onto the heap MUST BE MANUALLY DELETED
*/
char** allocateMatrix(const int rows, const int cols)
{ 
	char** grid = new char*[rows]; 
	for (int i = 0; i < rows; i++) {
		grid[i] = new char[cols];
	}
	return grid;
}

void deallocate_matrix(char **m, const int rows) 
{ 
    for(int i = 0; i < rows; i++) {
        delete [] m[i];
    }
    printf("oh\n");
    //delete [] m;
    printf("my\n");
    
}

// Send msg with end char !
void send_node(int cID, char *buff, int len) 
{
    len += 2;
    char s_buff[len];
    memset(s_buff, '\0', len);
    strncpy(s_buff, buff, (len - 2));
    strcat(s_buff, "!");
    send(cID, s_buff, len, 0);
}

// read until '!' or until "len" bytes are read
void recv_node(int cID, char *buff, int len)
{
    memset(buff, '\0', len);
    while (true) {
        char tempBuff[len];
        int b = recv(cID, tempBuff, len, 0);
        // check for '!'
        for (int i = 0; i <= b; i++) {
            if (tempBuff[i] == '!') {
                strncat(buff, tempBuff, i);
                return;
            }
        }
        strncat(buff, tempBuff, b);
    }
}

/*
*   Returns a matrix object givin a 2d chararacter array and its dimentions
*   Matrix object is loaded onto the heap MUST BE MANUALLY DELETED!
*/
Matrix* constructMatrix(char** matrix, const int width, const int height)
{
	// Format 2d char array in such a way that the client code can process (aka create a '*' border)
	for (int col = 0; col < width; col++)
	{
		for (int row = 0; row < height; row++)
		{
            // If border index set eq to '*'
			if (row == 0 || row == (height - 1) || col == 0 || col == (width - 1))
			{
				matrix[row][col] = '*';
			}
			else 
			{
				matrix[row][col] = ' ';
			}
		}
	}
	// Create matrix object
    Matrix* matrixObj = new Matrix(width, height);	
	// Store 2d char array into new matrix object
	matrixObj->setMatrix(matrix);
	return matrixObj;
}

// Structure that is passed into client thread
struct ClientData 
{
	// Socket descriptor
	int clientID;
	// Ref to 2d array of matrix objects
	Matrix** mMatrix;
	// Positon of matrix object in 2d array of matrix objects
	int posCol_in_matrix;
	int posRow_in_matrix;
	// Dimentions of 2d array of matrix objects
	int mMatrix_width;
	int mMatrix_height;
};

// print a 2D char array from matrix object
void printMatrix(Matrix** mMatrix, const int rows, const int cols)
{
	int subMatrixRow = 0;
	for (int row = 0; row < rows; row++)
	{
		for (int i = 0; i < mMatrix[0][0].getRows(); i++)
		{
			for (int col = 0; col < cols; col++)
			{
				Matrix* matrix = &mMatrix[row][col];
				for (int col2 = 0; col2 < matrix->getColums(); col2++)
				{
					printf("%c", matrix->getMatrix()[subMatrixRow][col2]);
				}
			}
			subMatrixRow++;
			printf("\n");	
		}
		subMatrixRow = 0;
	}
}

// print a 2D char array 
void print_2D_char_array(char** mMatrix, const int rows, const int cols)
{
	for (int row = 0; row < rows; row++)
	{
		for (int col = 0; col < cols; col++)
		{
		    printf("%c", mMatrix[row][col]);
		}
		printf("\n");	
	}
}

// Sync the matrix in server's memory to the node
void syncNode(struct ClientData* clientData) 
{
    // Get matrix object of this node
	Matrix* matrix = &clientData->mMatrix[clientData->posRow_in_matrix][clientData->posCol_in_matrix];
	// Send 2d char array matrix to node
	std::string str_matrix = "DATASET:" + matrix->toString() + "\0";
	send_node(clientData->clientID, (char*) str_matrix.c_str(), strlen(str_matrix.c_str())); 
	// Wait for node to respond with confimation that it recved its matrix
    int len = strlen("SYNC_DONE!\0");
    char nodeIn[len];
	recv_node(clientData->clientID, nodeIn, len);
    // On error exit
	if (strcmp(nodeIn, "SYNC_DONE") != 0) {
		printf("A catastrophic error has occured, all hope is lost.\nProc exiting...\n");
		exit(-1);
	}
}

// Gathers each node's matrix and puts them togeather into a 2D character array
char** reconnstructMasterMatrix(struct ClientData* clusterNodeData) 
{
    // matrixH / matrixW - 2 beacuse node matrix borders will not be added into the completed matrix
    int cMatrixH = (MASTER_MATRIX_H * (matrixH - 2));
    int cMatrixW = (MASTER_MATRIX_W * (matrixW - 2));
    char** cMatrix = allocateMatrix(cMatrixH, cMatrixW); 
    for (int i = 0; i < AMT_OF_NODES; i++) {
        char matrix_str[((matrixW * matrixH) + strlen("DATASET:\0")) + 1000];
        send_node(clusterNodeData[i].clientID, (char *) "REQDATASET\0", strlen("REQDATASET\0"));
        recv_node(clusterNodeData[i].clientID, matrix_str, (sizeof(matrix_str) / sizeof(char)));
        if (strncmp(matrix_str, "DATASET:", 8) == 0) {
            char* matrix_strP = &matrix_str[8];
            // add one to skip the '*' border
            int rowStart = (clusterNodeData[i].posRow_in_matrix * (matrixH - 2));
            int colStart = (clusterNodeData[i].posCol_in_matrix * (matrixW - 2));
            int i = matrixW;
            for (int row = rowStart; row < (rowStart + matrixH) - 2; row++) {
                for (int col = colStart; col < (colStart + matrixW) - 2; col++) {
                    // omit border characters
                    while (matrix_strP[i] == '*') 
                        i++;
                    cMatrix[row][col] = matrix_strP[i];
                    i++;
                }
            }
        } 
        else 
        {
            printf("corrupt data? -> on reconnstruction of master matrix\n");
        }
    }
    return cMatrix;
}

char** create_border(char** m, int h, int w) {
    w += 2;
    h += 2;
    char** newMat = allocateMatrix(h, w);
    for (int row = 0; row < h; row++) {
        for (int col = 0; col < w; col++) {
            if (row == 0 || row == (h - 1) || col == 0 || col == (w - 1)) {
                newMat[row][col] = '*';    
            } else {
                newMat[row][col] = m[row - 1][col - 1];
            }
        }
    }
    return newMat;
} 

static void* syncNodeRim(void* p) {
	struct ClientData* clientData = ((ClientData*) p);
    // Get nodes coresponding matrix object
	Matrix* matrix = &clientData->mMatrix[clientData->posRow_in_matrix][clientData->posCol_in_matrix];
	// Update matrix rim in serv mem
	send_node(clientData->clientID, (char *) "GETRIM", 6);
    int len = strlen("MATRIXRIM!\0") + ((matrix->getRows() * 2) + (matrix->getColums() * 2));
    char nodeIn[len];
	recv_node(clientData->clientID, nodeIn, len);
	if (strncmp(nodeIn, "MATRIXRIM", 9) != 0)
	{
		printf("this is bad very bad\n");
		printf("data read '%s'\n", nodeIn);
        exit(-1);
		pthread_exit(NULL);
	}
    // Set rim in server memory 
	matrix->setRim(&nodeIn[9]);
	pthread_exit(NULL);
}

// Function for mataining node on seperate thread. param -> ClientData struct declared near the top of this file
static void* stepNode(void *p)
{
	// Convert void pointer back into struct
	struct ClientData* clientData = ((ClientData*) p);
	Matrix* matrix = &clientData->mMatrix[clientData->posRow_in_matrix][clientData->posCol_in_matrix];
	// Buffer for handling socket output from node
	const int nodeIn_ln = (matrix->getRows() * matrix->getColums());
	char nodeIn[nodeIn_ln];
	// Begin step
	send_node(clientData->clientID, (char *) "STEP", 4);
	do 
	{
		memset(nodeIn, '\0', nodeIn_ln);
		recv_node(clientData->clientID, nodeIn, nodeIn_ln);
		// Node will request rims of surounding matrixs so it can acuratly complet simulation 
		if (strncmp(nodeIn, "GETRIMOF:", 9) == 0)
		{
			// Parse pos of matrix requested 
			char* cMatCords = &nodeIn[9];
			char cY[5];
			memset(cY, 0, 5);
			strncpy(cY, (const char*)cMatCords, (strlen(cMatCords) - strlen(strchr(cMatCords, '/'))));
			char* cX = &strchr(cMatCords, '/')[1];
			int row = atoi(cY);
			int col = atoi(cX);
			// Get matrix requested via offset from current node pos
			int posR = clientData->posRow_in_matrix + row;
			int posC = clientData->posCol_in_matrix + col;
			// If matrix is not a valid pos send back empty rim and prepare to resize matrix
			if (posR < 0 || posC < 0 || posR >= clientData->mMatrix_height || posC >= clientData->mMatrix_width)
			{
				// generate blank rim	
				int len = ((matrix->getRows() * 2) + (matrix->getColums() * 2));
				char rim[len];
				for (int i = 0; i < len; i++) {
					// ~ is used in this implementation of conways game of life to desinate undifined data 
					rim[i] = '~';
				}
				send_node(clientData->clientID, rim, len);
			}
			else 
			{
				// Send back requested rim
				int len = ((matrix->getRows() * 2) + (matrix->getColums() * 2));
				char rim[len];
				strcpy(rim, clientData->mMatrix[posR][posC].getRim());
				send_node(clientData->clientID, rim, len);
			}
		} else if (strcmp(nodeIn, "RESIZE_REQ") == 0) {
            // Set resize matrix flag to true
            pthread_mutex_lock(&rMutex);
	        resizeMatrix = true;	
            pthread_mutex_unlock(&rMutex);
        }
	} while(strncmp(nodeIn, "STEP_DONE", 9) != 0);
	pthread_exit(NULL);
}

int main() 
{
	// Keeps track of how many steps the nodes have taken
	int globalStep = 0;
	// Array of node data structs
	struct ClientData clusterClientData[AMT_OF_NODES];
	// Create 2d array of matrix object pointers
	Matrix** masterMatrix = new Matrix*[MASTER_MATRIX_H];	
	for (int row = 0; row < MASTER_MATRIX_H; row++)
	{
		masterMatrix[row] = new Matrix[MASTER_MATRIX_W];
	}
	// Assign and init matrix object
	for (int row = 0; row < MASTER_MATRIX_H; row++)
	{
		for (int col = 0; col < MASTER_MATRIX_W; col++)
		{
            // Create the matrix objects matrix in mem 
			char** matrix = allocateMatrix(matrixH, matrixW);
            // Assine values to the matrix array
			masterMatrix[row][col] = *constructMatrix(matrix, matrixW, matrixH);
            // Print it so we can see it worked
			masterMatrix[row][col].printMatrix();
		}
	}
	// init mutex for acessing shared data withen the threads
	if (pthread_mutex_init(&rMutex, NULL) != 0) {
		printf("failed to init rMutex\n");
		return -1;
	}

	// Start tcp sock serv <- program will act as a beowolf cluster meaning this process will be the master
    // and 'slave' computers will connect to this server and be used for processing 
	struct sockaddr_in socket_def;

	int sock = socket(AF_INET, SOCK_STREAM, 0);

	socket_def.sin_family = AF_INET;
	socket_def.sin_port = htons(9002);
	socket_def.sin_addr.s_addr = INADDR_ANY;

    // bind tcp socket
	if (bind(sock, (struct sockaddr*) &socket_def, sizeof(socket_def)) < 0)
	{
		printf("failed to bind socket.\n>.<\n");
		exit(-1);
	}
    // set socket to listen for incoming connections
	listen(sock, 1);

	// Pos of matrix object in matrix object 2d array the node will be assigned 
	int givinMatrixRow = 0;
	int givinMatrixCol = 0;

    char** grid = masterMatrix[0][2].getMatrix();
    // TODO read in start board from some output file 
	grid[3][6] = '#';
	grid[4][6] = '#';
	grid[5][6] = '#';
	grid[5][5] = '#';
	grid[4][4] = '#';
	
	/*grid[3][6] = '#';
	grid[3][5] = '#';
	grid[4][5] = '#';
	grid[5][5] = '#';
	grid[4][4] = '#';*/

	/*grid[6][5] = '#';
	grid[6][6] = '#';
	grid[6][7] = '#';*/
	
	// Loop untill all 12 nodes have connected
	while (true)
	{
		struct sockaddr_in client_addr;	
		socklen_t sock_len = sizeof(client_addr);
		// Listen for client
		int clientID = accept(sock, (struct sockaddr*) &client_addr, &sock_len);
		printf("got connection\n");
        // Set nodes matrix dimensions
        char dimen[30];
        memset(dimen, '\0', (sizeof(dimen) / sizeof(char)));
        sprintf(dimen, "SET_DIMEN:%d/%d", matrixH, matrixW);
        send_node(clientID, dimen, strlen(dimen));
		// Create struct of data that the node needs
		struct ClientData clientData;
		// Socket ID
		clientData.clientID = clientID;
		// Pointer to matrix segment
		clientData.mMatrix = masterMatrix;
		// Pos of matrix segment in master matrix
		clientData.posCol_in_matrix = givinMatrixCol;
		clientData.posRow_in_matrix = givinMatrixRow;
		// Dimentions of master matrix
		clientData.mMatrix_width = MASTER_MATRIX_W;
		clientData.mMatrix_height = MASTER_MATRIX_H;
		// Node step
		if (givinMatrixRow < 1) {
			clusterClientData[((givinMatrixRow * (MASTER_MATRIX_W - 1)) + givinMatrixCol)] = clientData;
		} else {
			clusterClientData[((givinMatrixRow * MASTER_MATRIX_W) + givinMatrixCol)] = clientData;
		}

		if (givinMatrixRow == 2 && givinMatrixCol != 3) {
			givinMatrixRow = 0;
			givinMatrixCol++;
			continue;
		}
		if (givinMatrixRow == 2 && givinMatrixCol == 3)
		{
			break;
		}
		givinMatrixRow++;
	}
	
    for (int i = 0; i < AMT_OF_NODES; i++)
	{
		syncNode(&clusterClientData[i]);
	}	

	pthread_t threads[AMT_OF_NODES];
	// Maintain timing of node steps 
	while (true)
	{
		// sync node rims
		for (int i = 0; i < AMT_OF_NODES; i++)
		{
			int rtn = -1;
			do {
				if ((rtn = pthread_create(&threads[i], NULL, syncNodeRim, &clusterClientData[i])) != 0) {
					printf("failed to create pthread on node (%i, %i)\n", clusterClientData[i].posRow_in_matrix, clusterClientData[i].posCol_in_matrix);
				}
			} while (rtn != 0);

		}
		// wait for all threads to finish
		for (int i = 0; i < AMT_OF_NODES; i++)
		{
			pthread_join(threads[i], NULL);	
		}
		// step nodes
		for (int i = 0; i < AMT_OF_NODES; i++)
		{
			int rtn = -1;
			do {
				if((rtn = pthread_create(&threads[i], NULL, stepNode, &clusterClientData[i])) != 0) {
					printf("failed to create pthread on node (%i, %i)\n", clusterClientData[i].posRow_in_matrix, clusterClientData[i].posCol_in_matrix);
				}
			} while (rtn != 0);
		}
		// wait for all threads to finish
		for (int i = 0; i < AMT_OF_NODES; i++)
		{
			pthread_join(threads[i], NULL);	
		}
        globalStep++;
        if (resizeMatrix) {
            // On resize consolidate sim to node 1,1 and continue sim with all other nodes blank
            int mH = (MASTER_MATRIX_H * (matrixH - 2));
            int mW = (MASTER_MATRIX_W * (matrixW - 2));
            char **m_temp = reconnstructMasterMatrix(clusterClientData);
            char **m = create_border(m_temp, mH, mW);
            matrixW = mW + 2;
            matrixH = mH + 2;
            print_2D_char_array(m, matrixH, matrixW);
            //deallocate_master_matrix(masterMatrix, (const int) MASTER_MATRIX_H);
            for (int row = 0; row < MASTER_MATRIX_H; row++) {
                for (int col = 0; col < MASTER_MATRIX_W; col++) {
                    masterMatrix[row][col].printMatrix();
                    if (row == 1 && col == 1) {
                        // Create matrix object
                        //Matrix* matrixObj = new Matrix(matrixW, matrixH);	
	                    // Store 2d char array into new matrix object
	                    //matrixObj->setMatrix(m);
                        deallocate_matrix(masterMatrix[row][col].getMatrix(), masterMatrix[row][col].getRows());
                        masterMatrix[row][col].setRows(matrixH);
                        masterMatrix[row][col].setColums(matrixW);
                        masterMatrix[row][col].setMatrix(m);
                        
                    } else {
                        char** tmp_matrix = allocateMatrix((const int) (matrixH - 2), (const int) (matrixW - 2));
                        for (int r = 0; r < (matrixH - 2); r++) {
                            for (int c = 0; c < (matrixW - 2); c++) {
                                tmp_matrix[r][c] = ' ';
                            }
                        }
                        char** matrix = create_border(tmp_matrix, matrixH - 2, matrixW - 2); 
                        char** oldM = masterMatrix[row][col].getMatrix();
                        print_2D_char_array(matrix, matrixH, matrixW);
                        deallocate_matrix(oldM, masterMatrix[row][col].getRows());
                        masterMatrix[row][col].setRows(matrixH);
                        masterMatrix[row][col].setColums(matrixW);
                        masterMatrix[row][col].setMatrix(matrix);
                    }
                    printf("%d/%d\n", row, col);
                    
                    masterMatrix[row][col].printMatrix();
                }
            } 
            char dimen[30];
            memset(dimen, '\0', (sizeof(dimen) / sizeof(char)));
            sprintf(dimen, "SET_DIMEN:%d/%d", matrixH, matrixW);
            for (int i = 0; i < AMT_OF_NODES; i++) {
                send_node(clusterClientData[i].clientID, dimen, strlen(dimen));
                syncNode(&clusterClientData[i]);
            }
            matrixArea = (matrixH * matrixW);
            resizeMatrix = false;
        }
        usleep(1000 * 100);
        if (globalStep == -20) {
			if ((globalStep % 100000) == 0)
			{
				printf("globalStep = %i\n", globalStep);
			}
			char** grid = clusterClientData[0].mMatrix[clusterClientData[0].posRow_in_matrix][clusterClientData[0].posCol_in_matrix].getMatrix();
			grid[2][1] = '#';
			grid[3][6] = '#';
			grid[4][6] = '#';
			grid[5][6] = '#';
			grid[5][5] = '#';
			grid[4][4] = '#';
			syncNode(&clusterClientData[0]);
		}
        // gather all node data and compile into one big array
        if ((globalStep % 10) == 0) {
            int mH = (MASTER_MATRIX_H * (matrixH - 2));
            int mW = (MASTER_MATRIX_W * (matrixW - 2));
            char **m_temp = reconnstructMasterMatrix(clusterClientData);
            char **m = create_border(m_temp, mH, mW);
            system("clear");
            print_2D_char_array(m, mH + 2, mW + 2); 
        }

	}
    pthread_mutex_destroy(&rMutex);
    return 0;
}
