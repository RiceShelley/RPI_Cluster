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
int matrix_w = 8;
int matrix_h = matrix_w;
int matrix_area = (matrix_h * matrix_w);

// Flag to signal that the sim needs more memory to continue
bool resize_matrix = false;

// Mutex for acessing shared data
pthread_mutex_t r_mutex;

/* 
 *   Returns 2d character array givin a width and height 
 *   array is loaded onto the heap MUST BE MANUALLY DELETED
 */
char **allocate_matrix(const int rows, const int cols)
{ 
	char **grid = new char*[rows]; 
	for (int i = 0; i < rows; i++) {
		grid[i] = new char[cols];
	}
	return grid;
}

/* 
 * Remove 2d array from heap
 */
void deallocate_matrix(char **m, const int rows) 
{ 
	for(int i = 0; i < rows; i++) {
		delete [] m[i];
	}
	printf("oh\n");
	//delete [] m;
	printf("my\n");
}

/*
 * All tcp msg's are ended with '!'
 * this helps reliablity on sending 
 * large amount of data
 */

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

// Read from sock until '!'
void recv_node(int cID, char *buff, int len)
{
	memset(buff, '\0', len);
	while (true) {
		char tempBuff[len];
		int b = recv(cID, tempBuff, len, 0);
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
Matrix *construct_matrix(char **matrix, const int width, const int height)
{
	for (int col = 0; col < width; col++) {
		for (int row = 0; row < height; row++) {
			if (row == 0 || row == (height - 1) || col == 0 || col == (width - 1))
				matrix[row][col] = '*';
			else
				matrix[row][col] = ' ';
		}
	}
	Matrix *matrixObj = new Matrix(width, height);	
	matrixObj->set_matrix(matrix);
	return matrixObj;
}

// Structure that is passed into client thread
struct client_data {
	// Socket descriptor
	int clientID;
	// Ref to 2d array of matrix objects
	Matrix **mMatrix;
	// Positon of matrix object in 2d array of matrix objects
	int posCol_in_matrix;
	int posRow_in_matrix;
	// Dimentions of 2d array of matrix objects
	int mMatrix_width;
	int mMatrix_height;
};

// Print a 2D char array from matrix object
void print_matrix(Matrix **mMatrix, const int rows, const int cols)
{
	int subMatrixRow = 0;
	for (int row = 0; row < rows; row++) {
		for (int i = 0; i < mMatrix[0][0].get_rows(); i++) {
			for (int col = 0; col < cols; col++) {
				Matrix *matrix = &mMatrix[row][col];
				for (int col2 = 0; col2 < matrix->get_colums(); col2++)
					printf("%c", matrix->get_matrix()[subMatrixRow][col2]);
			}
			subMatrixRow++;
			printf("\n");	
		}
		subMatrixRow = 0;
	}
}

// Print a 2D char array 
void print_2D_char_array(char **mMatrix, const int rows, const int cols)
{
	for (int row = 0; row < rows; row++)
	{
		for (int col = 0; col < cols; col++)
		    printf("%c", mMatrix[row][col]);
		printf("\n");	
	}
}

// Sync the matrix in server's memory to the node
void sync_node(struct client_data *clientData) 
{
	Matrix *matrix = &clientData->mMatrix[clientData->posRow_in_matrix][clientData->posCol_in_matrix];
	std::string str_matrix = "DATASET:" + matrix->to_string() + "\0";
	send_node(clientData->clientID, (char *) str_matrix.c_str(), strlen(str_matrix.c_str())); 
	int len = strlen("SYNC_DONE!\0");
	char nodeIn[len];
	recv_node(clientData->clientID, nodeIn, len);
	if (strcmp(nodeIn, "SYNC_DONE") != 0) {
		printf("A catastrophic error has occured, all hope is lost.\nProc exiting...\n");
		exit(-1);
	}
}

// Gathers each node's matrix and puts them togeather into a 2D character array
char **reconnstruct_master_matrix(struct client_data *clusterNodeData) 
{
	// matrix_h / matrix_w - 2 beacuse node matrix borders will not be added into the completed matrix
	int cMatrixH = (MASTER_MATRIX_H * (matrix_h - 2));
	int cMatrixW = (MASTER_MATRIX_W * (matrix_w - 2));
	char **cMatrix = allocate_matrix(cMatrixH, cMatrixW); 
	for (int i = 0; i < AMT_OF_NODES; i++) {
		char matrix_str[((matrix_w * matrix_h) + strlen("DATASET:\0")) + 1000];
		send_node(clusterNodeData[i].clientID, (char *) "REQDATASET\0", strlen("REQDATASET\0"));
		recv_node(clusterNodeData[i].clientID, matrix_str, (sizeof(matrix_str) / sizeof(char)));
		if (strncmp(matrix_str, "DATASET:", 8) == 0) {
			char *matrix_strP = &matrix_str[8];
			int rowStart = (clusterNodeData[i].posRow_in_matrix * (matrix_h - 2));
			int colStart = (clusterNodeData[i].posCol_in_matrix * (matrix_w - 2));
			int i = matrix_w;
			for (int row = rowStart; row < (rowStart + matrix_h) - 2; row++) {
				for (int col = colStart; col < (colStart + matrix_w) - 2; col++) {
					while (matrix_strP[i] == '*') 
						i++;
					cMatrix[row][col] = matrix_strP[i];
					i++;
				}
			}
		} else 
			printf("corrupt data? -> on reconnstruction of master matrix\n");
	}
	return cMatrix;
}

char **create_border(char **m, int h, int w) 
{
	w += 2;
	h += 2;
	char **newMat = allocate_matrix(h, w);
	for (int row = 0; row < h; row++) {
		for (int col = 0; col < w; col++) {
			if (row == 0 || row == (h - 1) || col == 0 || col == (w - 1))
				newMat[row][col] = '*';    
			else
				newMat[row][col] = m[row - 1][col - 1];
		}
	}
	return newMat;
} 

/*
 * Update matrix rim in server mem 
 * *p = client data struct
 */

static void *sync_node_rim(void *p) 
{
	struct client_data* clientData = ((client_data*) p);
	Matrix *matrix = &clientData->mMatrix[clientData->posRow_in_matrix][clientData->posCol_in_matrix];
	send_node(clientData->clientID, (char *) "GETRIM", 6);
	int len = strlen("MATRIXRIM!\0") + ((matrix->get_rows() * 2) + (matrix->get_colums() * 2));
	char nodeIn[len];
	recv_node(clientData->clientID, nodeIn, len);
	if (strncmp(nodeIn, "MATRIXRIM", 9) != 0) {
		printf("this is bad very bad\n");
		printf("data read '%s'\n", nodeIn);
		exit(-1);
	}
	matrix->set_rim(&nodeIn[9]);
	pthread_exit(NULL);
}

// Function for mataining node on seperate thread. param -> client_data struct declared near the top of this file
static void *step_node(void *p)
{
	struct client_data* clientData = ((client_data*) p);
	Matrix *matrix = &clientData->mMatrix[clientData->posRow_in_matrix][clientData->posCol_in_matrix];
	const int nodeIn_ln = (matrix->get_rows() * matrix->get_colums());
	char nodeIn[nodeIn_ln];
	send_node(clientData->clientID, (char *) "STEP", 4);
	do {
		memset(nodeIn, '\0', nodeIn_ln);
		recv_node(clientData->clientID, nodeIn, nodeIn_ln);
		if (strncmp(nodeIn, "GETRIMOF:", 9) == 0) {
			// Parse pos of matrix requested 
			char *cMatCords = &nodeIn[9];
			char cY[5];
			memset(cY, 0, 5);
			strncpy(cY, (const char *) cMatCords, (strlen(cMatCords) - strlen(strchr(cMatCords, '/'))));
			char *cX = &strchr(cMatCords, '/')[1];
			int row = atoi(cY);
			int col = atoi(cX);
			// Get matrix requested via offset from current node pos
			int posR = clientData->posRow_in_matrix + row;
			int posC = clientData->posCol_in_matrix + col;
			// If matrix is not a valid pos send back empty rim and prepare to resize matrix
			if (posR < 0 || posC < 0 || posR >= clientData->mMatrix_height || posC >= clientData->mMatrix_width) {
				// generate blank rim	
				int len = ((matrix->get_rows() * 2) + (matrix->get_colums() * 2));
				char rim[len];
				// ~ is used in this implementation of conways game of life to desinate undifined data 
				for (int i = 0; i < len; i++)
					rim[i] = '~';
				send_node(clientData->clientID, rim, len);
			} else {
				int len = ((matrix->get_rows() * 2) + (matrix->get_colums() * 2));
				char rim[len];
				strcpy(rim, clientData->mMatrix[posR][posC].get_rim());
				send_node(clientData->clientID, rim, len);
			}
		} else if (strcmp(nodeIn, "RESIZE_REQ") == 0) {
			pthread_mutex_lock(&r_mutex);
			resize_matrix = true;	
			pthread_mutex_unlock(&r_mutex);
		}
	} while(strncmp(nodeIn, "STEP_DONE", 9) != 0);
	pthread_exit(NULL);
}

int main() 
{
	// Keeps track of how many steps the nodes have taken
	int globalStep = 0;
	
	struct client_data n_data[AMT_OF_NODES];
	
	Matrix **master_m = new Matrix*[MASTER_MATRIX_H];

	for (int row = 0; row < MASTER_MATRIX_H; row++)
		master_m[row] = new Matrix[MASTER_MATRIX_W];

	for (int row = 0; row < MASTER_MATRIX_H; row++) {
		for (int col = 0; col < MASTER_MATRIX_W; col++) {
			char **matrix = allocate_matrix(matrix_h, matrix_w);
			master_m[row][col] = *construct_matrix(matrix, matrix_w, matrix_h);
			master_m[row][col].print_matrix();
		}
	}
	
	if (pthread_mutex_init(&r_mutex, NULL) != 0) {
		printf("failed to init r_mutex\n");
		return -1;
	}
	
	struct sockaddr_in socket_def;

	int sock = socket(AF_INET, SOCK_STREAM, 0);

	socket_def.sin_family = AF_INET;
	socket_def.sin_port = htons(9002);
	socket_def.sin_addr.s_addr = INADDR_ANY;

	if (bind(sock, (struct sockaddr*) &socket_def, sizeof(socket_def)) < 0) {
		printf("failed to bind socket.\n>.<\n");
		exit(-1);
	}

	listen(sock, 1);

	// Pos of matrix object in matrix object 2d array the node will be assigned 
	int given_row = 0;
	int given_col = 0;

	char **grid = master_m[0][2].get_matrix();
	// TODO read in start board from some output file 
	grid[3][6] = '#';
	grid[4][6] = '#';
	grid[5][6] = '#';
	grid[5][5] = '#';
	grid[4][4] = '#';
	
	// Loop untill all 12 nodes have connected
	while (true) {
		struct sockaddr_in client_addr;	
		socklen_t sock_len = sizeof(client_addr);
		
		int s_fd = accept(sock, (struct sockaddr*) &client_addr, &sock_len);
		printf("got connection\n");
		
		char dimen[30];
		memset(dimen, '\0', (sizeof(dimen) / sizeof(char)));
		sprintf(dimen, "SET_DIMEN:%d/%d", matrix_h, matrix_w);
		send_node(sock, dimen, strlen(dimen));
		
		struct client_data clientData;
		clientData.clientID = s_fd;
		clientData.mMatrix = master_m;
		clientData.posCol_in_matrix = given_col;
		clientData.posRow_in_matrix = given_row;
		clientData.mMatrix_width = MASTER_MATRIX_W;
		clientData.mMatrix_height = MASTER_MATRIX_H;
		
		if (given_row < 1)
			n_data[((given_row * (MASTER_MATRIX_W - 1)) + given_col)] = clientData;
		else
			n_data[((given_row * MASTER_MATRIX_W) + given_col)] = clientData;

		if (given_row == 2) {
			if (given_col == 3)
				break;
			else {
				given_row = 0;
				given_col++;
				continue;
			}
		}
		given_row++;
	}
	
	for (int i = 0; i < AMT_OF_NODES; i++)
		sync_node(&n_data[i]);

	pthread_t threads[AMT_OF_NODES];	

	while (true) {
		// Sync node rims
		for (int i = 0; i < AMT_OF_NODES; i++) {
			int rtn = -1;
			do {
				if ((rtn = pthread_create(&threads[i], NULL, sync_node_rim, &n_data[i])) != 0)
					printf("failed to create pthread on node (%i, %i)\n", n_data[i].posRow_in_matrix, n_data[i].posCol_in_matrix);
			} while (rtn != 0);
		}
		
		for (int i = 0; i < AMT_OF_NODES; i++)
			pthread_join(threads[i], NULL);	

		// Step nodes
		for (int i = 0; i < AMT_OF_NODES; i++) {
			int rtn = -1;
			do {
				if((rtn = pthread_create(&threads[i], NULL, step_node, &n_data[i])) != 0)
					printf("failed to create pthread on node (%i, %i)\n", n_data[i].posRow_in_matrix, n_data[i].posCol_in_matrix);
			} while (rtn != 0);
		}
		
		for (int i = 0; i < AMT_OF_NODES; i++)
			pthread_join(threads[i], NULL);	

		globalStep++;
		
		if (resize_matrix) {
			// On resize consolidate sim to node 1,1 and continue sim with all other nodes blank
			int mH = (MASTER_MATRIX_H * (matrix_h - 2));
			int mW = (MASTER_MATRIX_W * (matrix_w - 2));
			char **m_temp = reconnstruct_master_matrix(n_data);
			deallocate_matrix(m_temp, mH);
			char **m = create_border(m_temp, mH, mW);
			matrix_w = mW + 2;
			matrix_h = mH + 2;
			print_2D_char_array(m, matrix_h, matrix_w);
			for (int row = 0; row < MASTER_MATRIX_H; row++) {
				for (int col = 0; col < MASTER_MATRIX_W; col++) {
					if (row == 1 && col == 1) {
						deallocate_matrix(master_m[row][col].get_matrix(), master_m[row][col].get_rows());
						master_m[row][col].set_rows(matrix_h);
						master_m[row][col].set_colums(matrix_w);
						master_m[row][col].set_matrix(m);
					} else {
						char **tmp_matrix = allocate_matrix((const int) (matrix_h - 2), (const int) (matrix_w - 2));
						for (int r = 0; r < (matrix_h - 2); r++) {
							for (int c = 0; c < (matrix_w - 2); c++)
								tmp_matrix[r][c] = ' ';
						}
						char **matrix = create_border(tmp_matrix, matrix_h - 2, matrix_w - 2); 
						char **oldM = master_m[row][col].get_matrix();
						print_2D_char_array(matrix, matrix_h, matrix_w);
						deallocate_matrix(oldM, master_m[row][col].get_rows());
						master_m[row][col].set_rows(matrix_h);
						master_m[row][col].set_colums(matrix_w);
						master_m[row][col].set_matrix(matrix);
					}
					master_m[row][col].print_matrix();
				}
			} 
			char dimen[30];
			memset(dimen, '\0', (sizeof(dimen) / sizeof(char)));
			sprintf(dimen, "SET_DIMEN:%d/%d", matrix_h, matrix_w);
			for (int i = 0; i < AMT_OF_NODES; i++) {
				send_node(n_data[i].clientID, dimen, strlen(dimen));
				sync_node(&n_data[i]);
			}
			matrix_area = (matrix_h * matrix_w);
			resize_matrix = false;
		}
		usleep(1000 * 100);
		if (globalStep == -20) {
			if ((globalStep % 100000) == 0)
				printf("globalStep = %i\n", globalStep);
			char **grid = n_data[0].mMatrix[n_data[0].posRow_in_matrix][n_data[0].posCol_in_matrix].get_matrix();
			grid[2][1] = '#';
			grid[3][6] = '#';
			grid[4][6] = '#';
			grid[5][6] = '#';
			grid[5][5] = '#';
			grid[4][4] = '#';
			sync_node(&n_data[0]);
		}
		// gather all node data and compile into one big array
		if ((globalStep % 10) == 0) {
			int mH = (MASTER_MATRIX_H * (matrix_h - 2));
			int mW = (MASTER_MATRIX_W * (matrix_w - 2));
			char **m_temp = reconnstruct_master_matrix(n_data);
			char **m = create_border(m_temp, mH, mW);
			system("clear");
			print_2D_char_array(m, mH + 2, mW + 2); 
		}
	}
	pthread_mutex_destroy(&r_mutex);
	return 0;
}
