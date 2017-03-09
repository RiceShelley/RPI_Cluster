/*
 * =====================================================================================
 *
 *       Filename:  Main.c
 *
 *    Description:  game of life 
 *
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

#define GRID_WIDTH 100
#define GRID_HEIGHT 50

char grid[GRID_HEIGHT][GRID_WIDTH]; 
char tempGrid[GRID_HEIGHT][GRID_WIDTH];

void clear_grid();
void print_grid();
void step();
int test_cell(int row, int col);

bool continue_step = true;

int main() 
{
	// init grid by clearing
	clear_grid();	
	grid[3][3] = '#';
	grid[4][4] = '#';
	grid[5][4] = '#';
	grid[5][3] = '#';
	grid[5][2] = '#';
	// step through grid
	for (int i = 0; i < 200; i++) {
		usleep(1000 * 250);
		print_grid();
		step();
		print_grid();
		printf("\n\n");
		if (!continue_step)
			break;
	}
	printf("sim done.\n");
}

void step() 
{
	// Init temp grid to equal current grid
	for (int row = 0; row < GRID_HEIGHT; row++) {
		for (int col = 0; col < GRID_WIDTH; col++)
			tempGrid[row][col] = grid[row][col];
	}
	// Apply step logic to grid
	for (int row = 0; row < GRID_HEIGHT; row++) {
		for (int col = 0; col < GRID_WIDTH; col++) {
			int neighbours = test_cell(row, col);
			if (neighbours == 2 || neighbours == 3) {
				if ((grid[row][col] == ' ' || grid[row][col] == '*') && neighbours == 3) {
					tempGrid[row][col] = '#';	
					continue_step = !(row == 0 || row == (GRID_HEIGHT - 1) || col == 0 || col == (GRID_WIDTH - 1));
					printf("row = %d, col = %d\n", row, col);
				}
			} else {
				if (grid[row][col] != '*')
					tempGrid[row][col] = ' ';
			}
		}
	}
	// set grid to new step
	for (int row = 0; row < GRID_HEIGHT; row++) {
		for (int col = 0; col < GRID_WIDTH; col++)
			grid[row][col] = tempGrid[row][col];
	}
}

int test_cell(int row, int col)
{
	// do neighbour count
	int neighbours = 0;
	if (grid[row + 1][col - 1] == '#') 
		neighbours++;
	if (grid[row][col - 1] == '#')
		neighbours++;
	if (grid[row - 1][col - 1] == '#')
		neighbours++;
	if (grid[row - 1][col] == '#')
		neighbours++;
	if (grid[row - 1][col + 1] == '#')
		neighbours++;
	if (grid[row][col + 1] == '#')
		neighbours++;
	if (grid[row + 1][col + 1] == '#')
		neighbours++;
	if (grid[row + 1][col] == '#')
		neighbours++;
	return neighbours;
}

void clear_grid()
{
	for (int row = 0; row < GRID_HEIGHT; row++) {
		for (int col = 0; col < GRID_WIDTH; col++) {
			if (row == 0 || row == (GRID_HEIGHT - 1) || col == 0 || col == (GRID_WIDTH -1)) 
				grid[row][col] = '*';
			else
				grid[row][col] = ' ';
		}
	}
}

void print_grid() 
{
	for (int row = 0; row < GRID_HEIGHT; row++) {
		for (int col = 0; col < GRID_WIDTH; col++)
			printf("%c", grid[row][col]);
		printf("\n");
	}
}
