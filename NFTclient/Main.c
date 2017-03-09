/*
 * =====================================================================================
 *
 *       Filename:  Main.c
 *
 *    Description:  game of life *
 *        Version:  1.0
 *        Created:  08/07/2016 03:10:27 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Rice Shelley
 *   Organization:  
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <string.h>
#include <resolv.h>
#include <errno.h>

// Board dimensions <- set by server 
int grid_w = 5;
int grid_h = 5;
int gridA = 25;

// 2d array of chars for holding game of life board
char **grid;
char **tempGrid;

// flag requesting server matrix resize
bool resize_req = false;

// Socket descriptor
int sock_fd = 0;

void write_matrix(char *newMatrix)
{
	int i = 0;
	for (int row = 0; row < grid_h; row++) {
		for (int col = 0; col < grid_w; col++) {
			grid[row][col] = newMatrix[i];
			i++;
		}
	}	
}

/* 
*   Returns 2d character array given a width and height 
*   array is loaded onto the heap MUST BE MANUALLY DELETED
*/
char **allocate_matrix(int width, int height)
{ 
	char **grid = calloc((height + 1), sizeof(char *)); 
	for (int i = 0; i < (height + 1); i++)
		grid[i] = calloc((width + 1), sizeof(char));
	return grid;
}

void deallocate_matrix(char **matrix, int height) 
{
	for (int i = 0; i < (height + 1); i++) 
		free(matrix[i]);
	free(matrix);
}

void send_server(int c, char *buff, int len) 
{
	len += 2;
	char s_buff[len];
	memset(s_buff, '\0', len);
	strcpy(s_buff, buff);
	strcat(s_buff, "!");
	send(c, s_buff, len, 0);
}

// read until '!' or until "len" bytes are read
void recv_server(int c, char *buff, int len)
{
	memset(buff, '\0', len);
	while (true) {
		char tempBuff[len];
		int b = recv(c, tempBuff, len, 0);
		for (int i = 0; i < b; i++) {
			if (tempBuff[i] == '!') {
				strncat(buff, tempBuff, i);
				return;
			}
		}
		strncat(buff, tempBuff, b);
	}
}

char *get_matrix_rim()
{
	char *rim = (char* ) malloc(((grid_w * 2) + (grid_h * 2) + 1));
	int i = 0;
	int col = 0;
	int row = 1;
	// assign top then bottom rows followed by left then right
	for (int n = 0; n < 2; n++) {
		for (col = 0; col < grid_w; col++) {
			rim[i] = grid[row][col];
			i++;
		}
		row = (grid_h - 2);
	}
	col = 1;
	for (int n = 0; n < 2; n++) {
		for (row = 0; row < grid_h; row++) {
			rim[i] = grid[row][col];
			i++;
		}
		col = (grid_w - 2);
	}
	rim[i] = '\0';
	return rim;
}

void clear_grid()
{
	for (int row = 0; row < grid_h; row++) {
		for (int col = 0; col < grid_w; col++) {
			if (row == 0 || row == (grid_h - 1) || col == 0 || col == (grid_w -1)) 
				grid[row][col] = '*';
			else
				grid[row][col] = ' ';
		}
	}
}

void print_2d_array(char **grid, int rows, int cols) 
{
	for (int row = 0; row < rows; row++) {
		for (int col = 0; col < cols; col++)
			printf("%c", grid[row][col]);
		printf("\n");
	}
}

int testCell(int row, int col, char **tempGrid)
{
	int neighbours = 0;
	if (tempGrid[row][col] == '#') {
		if ((col == 2 && (row > 1 && row < grid_h - 2)) || 
		(col == grid_w - 3 && (row > 1 && row < grid_h)) ||
		(row == 2 && (col > 1 && col < grid_w - 2)) || 
		(row == grid_h - 3 && (col > 1 && col < grid_w - 2))) {
			/* 
			* do check for undefined data character
			* undefined data could effect the sim if a live cell nears it 
			* in the case of a live cell near a undefined data character
			* the board needs to be resized to allow the sim to acuratly continue
			*/
			if (tempGrid[row + 2][col - 2] == '~') {
				resize_req = true;
			} else if (tempGrid[row][col - 2] == '~') {
				resize_req = true;
			} else if (tempGrid[row - 2][col - 2] == '~') {
				resize_req = true;
			} else if (tempGrid[row - 2][col] == '~') {
				resize_req = true;
			} else if (tempGrid[row - 2][col + 2] == '~') {
				resize_req = true;
			} else if (tempGrid[row][col + 2] == '~') { 
				resize_req = true;
			} else if (tempGrid[row + 2][col + 2] == '~') {
				resize_req = true;
			} else if (tempGrid[row + 2][col] == '~') {
				resize_req = true;	
			}
		}
	}
	if (tempGrid[row + 1][col - 1] == '#') 
		neighbours++;
	if (tempGrid[row][col - 1] == '#')
		neighbours++;
	if (tempGrid[row - 1][col - 1] == '#')
		neighbours++;
	if (tempGrid[row - 1][col] == '#')
		neighbours++;
	if (tempGrid[row - 1][col + 1] == '#')
		neighbours++;
	if (tempGrid[row][col + 1] == '#')
		neighbours++;
	if (tempGrid[row + 1][col + 1] == '#')
		neighbours++;
	if (tempGrid[row + 1][col] == '#')
		neighbours++;
	return neighbours;
}

void step()
{
	// init temp grid to equal current grid
	char rimN1_Z[(grid_w * 2) + (grid_h * 2)];
	char rim1_Z[(grid_w * 2) + (grid_h * 2)];
	char rimZ_N1[(grid_w * 2) + (grid_h * 2)];
	char rimZ_1[(grid_w * 2) + (grid_h * 2)];
	char rimN1_N1[(grid_w * 2) + (grid_h * 2)];
	char rimN1_1[(grid_w * 2) + (grid_h * 2)];
	char rim1_1[(grid_w * 2) + (grid_h * 2)];
	char rim1_N1[(grid_w * 2) + (grid_h * 2)];

	// copy current board into temp array
	for (int row = 0; row < grid_h; row++) {
		for (int col = 0; col < grid_w; col++)
			tempGrid[row][col] = grid[row][col];
	}

	// get rim of matrix above this one
	send_server(sock_fd, "GETRIMOF:-1/0", (sizeof("GETRIMOF:-1/0") / sizeof(char)));
	memset(rimN1_Z, 0, (sizeof(rimN1_Z) / sizeof(char)));
	recv_server(sock_fd, rimN1_Z, (sizeof(rimN1_Z) / sizeof(char)));
	char bottomRow[grid_w];
	memset(bottomRow, 0, grid_w);
	strcpy(rimN1_Z, (const char*) &rimN1_Z[grid_w]);
	strncpy(bottomRow, rimN1_Z, grid_w);
	for (int i = 0; i < grid_w; i++)
		grid[0][i] = bottomRow[i];

	// get rim of matrix bellow this one
	send_server(sock_fd, "GETRIMOF:1/0", (sizeof("GETRIMOF:1/0") / sizeof(char)));
	memset(rim1_Z, 0, (sizeof(rim1_Z) / sizeof(char)));
	recv_server(sock_fd, rim1_Z, (sizeof(rim1_Z) / sizeof(char)));
	char topRow[grid_w];
	memset(topRow, 0, grid_w);
	strncpy(topRow, rim1_Z, grid_w);
	for (int i = 0; i < grid_w; i++)
		grid[grid_h - 1][i] = topRow[i];

	// get rim of matrix to the left of this one
	send_server(sock_fd, "GETRIMOF:0/-1", (sizeof("GETRIMOF:0/-1") / sizeof(char)));
	memset(rimZ_N1, 0, (sizeof(rimZ_N1) / sizeof(char)));
	recv_server(sock_fd, rimZ_N1, (sizeof(rimZ_N1) / sizeof(char)));
	char rightRow[grid_h];
	memset(rightRow, 0, grid_h);
	strcpy(rimZ_N1, (const char*) &rimZ_N1[((grid_w * 2) + grid_h)]);
	strcpy(rightRow, rimZ_N1);
	for (int i = 0; i < grid_h; i++)
		grid[i][0] = rightRow[i];

	// get rim of matrix to the right of this one
	send_server(sock_fd, "GETRIMOF:0/1", (sizeof("GETRIMOF:0/1") / sizeof(char)));
	memset(rimZ_1, 0, (sizeof(rimZ_1) / sizeof(char)));
	recv_server(sock_fd, rimZ_1, (sizeof(rimZ_1) / sizeof(char)));
	char leftRow[grid_h];
	memset(leftRow, 0, grid_h);
	strncpy(leftRow, (const char*) &rimZ_1[(grid_w * 2)], grid_h);
	for (int i = 0; i < grid_h; i++) 
		grid[i][grid_w - 1] = leftRow[i];

	// Add edges of surrounding matrices

	// get rim of matrix above and the the left
	send_server(sock_fd, "GETRIMOF:-1/-1", (sizeof("GETRIMOF:-1/-1") / sizeof(char)));
	memset(rimN1_N1, 0, (sizeof(rimN1_N1) / sizeof(char)));
	recv_server(sock_fd, rimN1_N1, (sizeof(rimN1_N1) / sizeof(char)));
	// top left corner
	grid[0][0] = rimN1_N1[(grid_w * 2) - 2];

	// get rim of matrix above and to the right
	send_server(sock_fd, "GETRIMOF:-1/1", (sizeof("GETRIMOF:-1/1") / sizeof(char)));
	memset(rimN1_1, 0, (sizeof(rimN1_1) / sizeof(char)));
	recv_server(sock_fd, rimN1_1, (sizeof(rimN1_1) / sizeof(char)));
	// top right corner
	grid[0][grid_w - 1] = rimN1_1[grid_w + 1];

	// get rim of matrix bellow and to the right
	send_server(sock_fd, "GETRIMOF:1/1", (sizeof("GETRIMOF:1/1") / sizeof(char)));
	memset(rim1_1, 0, (sizeof(rim1_1) / sizeof(char)));
	recv_server(sock_fd, rim1_1, (sizeof(rim1_1) / sizeof(char)));
	// bot right corner
	grid[grid_h - 1][grid_w - 1] = rim1_1[1];

	// get rim of matrix bellow and to the left
	send_server(sock_fd, "GETRIMOF:1/-1", (sizeof("GETRIMOF:1/-1") / sizeof(char)));
	memset(rim1_N1, 0, (sizeof(rim1_N1) / sizeof(char)));
	recv_server(sock_fd, rim1_N1, (sizeof(rim1_N1) / sizeof(char)));
	// bot left corner
	grid[grid_h - 1][0] = rim1_N1[grid_w - 2];
 
	// Apply logic to grid
	for (int row = 1; row < grid_h - 1; row++) {
		for (int col = 1; col < grid_w - 1; col++) {
			int neighbours = testCell(row, col, grid);
			if (neighbours == 2 || neighbours == 3) {
				if (grid[row][col] == ' ' && neighbours == 3)
					tempGrid[row][col] = '#';	
			} else
				tempGrid[row][col] = ' ';
		}
	}

	// Set grid to contents of temp grid
	for (int row = 0; row < grid_h; row++) {
		for (int col = 0; col < grid_w; col++)
			grid[row][col] = tempGrid[row][col];
	}
	if (resize_req) {
		send_server(sock_fd, "RESIZE_REQ", 10);
		resize_req = false;
	}
	send_server(sock_fd, "STEP_DONE", 9);	
}

void proc_contol()
{
	struct sockaddr_in serv_def;
	int portNum = 9002;
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	serv_def.sin_family = AF_INET;
	serv_def.sin_port = htons(portNum);
	inet_aton((const char *) "192.168.1.10", &serv_def.sin_addr.s_addr);
	
	connect(sock_fd, (struct sockaddr *) &serv_def, sizeof(serv_def));
	
	int buff_pad = 100;

	while(true) {
		int fromS_ln = gridA + buff_pad;
		char fromS[fromS_ln];
		recv_server(sock_fd, fromS, fromS_ln);
		if (strncmp(fromS, "STEP", 4) == 0)
			step();
		// Server has sent node a new matrix
		else if (strncmp(fromS, "DATASET:", 8) == 0) {
			char *dataSet = &fromS[8];
			write_matrix(dataSet);
			send_server(sock_fd, "SYNC_DONE", 9);
		}
		// Server is resizing matrix
		else if (strncmp(fromS, "SET_DIMEN:", 10) == 0) {
			// Free mem from old grids 
			deallocate_matrix(grid, grid_h);
			deallocate_matrix(tempGrid, grid_h);
			// Parse dimentions of grid
			char *dimen = &fromS[10]; 
			char cY[5];
			memset(cY, 0, 5);
			strncpy(cY, (const char*)dimen, (strlen(dimen) - strlen(strchr(dimen, '/'))));
			char *cX = &strchr(dimen, '/')[1];
			grid_h = atoi(cY);
			grid_w = atoi(cX);
			gridA = (grid_h * grid_w);
			// Create new grids with new dimensions
			grid = allocate_matrix(grid_w, grid_h);
			tempGrid = allocate_matrix(grid_w, grid_h);
			clear_grid();
		}
		/*
		 * server has reqested the matrix this node has
		 * been processing!!!
		 * possible resons: 
		 * - server is building a master matrix of the current point in the simulation
		 */
		else if (strncmp(fromS, "REQDATASET", 10) == 0) {
			char matrix_str[((grid_w * grid_h) + 13)];
			strcpy(matrix_str, "DATASET:");
			int i = 8;
			for (int row = 0; row < grid_h; row++) {
				for (int col = 0; col < grid_w; col++) {
					matrix_str[i] = grid[row][col];
					i++;
				}
			}
			matrix_str[i] = '\0';
			send_server(sock_fd, matrix_str, strlen(matrix_str));
		}
		/* 
		 * server has requested the rim of this matrix
		 * reason: another node processing an adjacent 
		 * matrix needs to know the postion of cells 
		 * that could possibly affect its simulation
		 */
		else if (strncmp(fromS, "GETRIM", 6) == 0) {
			char *rim = get_matrix_rim();
			char *rimFormat = (char*) malloc(((int) strlen(rim)) + ((int) strlen("MATRIXRIM\0")) + 1);
			strcpy(rimFormat, ((const char*) "MATRIXRIM"));
			strcat(rimFormat, ((const char*) rim));
			send_server(sock_fd, rimFormat, strlen(rimFormat));
			free(rimFormat);
			free(rim);
		} else if (strcmp(fromS, "") == 0)
			exit(1);
	}
}

int main() 
{
	grid = allocate_matrix(grid_w, grid_h);
	tempGrid = allocate_matrix(grid_w, grid_h);
	clear_grid();	
	proc_contol();
}
