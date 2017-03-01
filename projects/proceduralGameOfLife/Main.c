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

#define GRID_WIDTH 20
#define GRID_HEIGHT 30

char grid[GRID_HEIGHT][GRID_WIDTH]; 
char tempGrid[GRID_HEIGHT][GRID_WIDTH];

int gridW = GRID_WIDTH;
int gridH = GRID_HEIGHT;

void clearGrid();
void printGrid();
void step();
int testCell(int row, int col);

bool continueStep = true;

int main() 
{
	// init grid by clearing
	clearGrid();	
	grid[3][3] = '#';
	grid[4][4] = '#';
	grid[5][4] = '#';
	grid[5][3] = '#';
	grid[5][2] = '#';
	// step through grid
	for (int i = 0; i < 80; i++)
	{
		usleep(1000 * 250);
		printGrid();
		step();
		printGrid();
		printf("\n\n");
		if (!continueStep)
		{
			break;
		}
	}
	printf("sim done.\n");
}

void step()
{
	// init temp grid to equal current grid
	for (int row = 0; row < gridH; row++)
	{
		for (int col = 0; col < gridW; col++)
		{
			tempGrid[row][col] = grid[row][col];
		}
	}
	// apply step logic to grid
	for (int row = 0; row < gridH; row++)
	{
		for (int col = 0; col < gridW; col++)
		{
				int neighbours = testCell(row, col);
				if (neighbours == 2 || neighbours == 3)
				{
					if ((grid[row][col] == ' ' || grid[row][col] == '*') && neighbours == 3)
					{
						tempGrid[row][col] = '#';	
						printf("row = %d, col = %d\n", row, col);
						if (row == 0 || row == (gridH - 1) || col == 0 || col == (gridW - 1)) 
						{
							printf("wat\n");
							continueStep = false;
						}
					}
				}
				else 
				{
					if (grid[row][col] != '*') 
					{	
						tempGrid[row][col] = ' ';
					}
				}
		}
	}
	// set grid to new step
	for (int row = 0; row < gridH; row++)
	{
		for (int col = 0; col < gridW; col++)
		{
			grid[row][col] = tempGrid[row][col];
		}
	}
}

int testCell(int row, int col)
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

void clearGrid()
{
	for (int row = 0; row < gridH; row++)
	{
		for (int col = 0; col < gridW; col++)
		{
			if (row == 0 || row == (gridH - 1) || col == 0 || col == (gridW -1)) 
				grid[row][col] = '*';
			else
				grid[row][col] = ' ';
		}
	}
}

void printGrid() 
{
	for (int row = 0; row < gridH; row++)
	{
		for (int col = 0; col < gridW; col++)
		{
			printf("%c", grid[row][col]);
		}
		printf("\n");
	}
}

