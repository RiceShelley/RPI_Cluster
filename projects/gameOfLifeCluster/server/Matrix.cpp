#include "Matrix.hpp"

Matrix::Matrix(int width, int hieght) : rows(hieght), colums(width) {
	printf("Matrix created\n");
	matrixRim = new char[(rows * 2) + (colums * 2)];
}

Matrix::Matrix() {
	printf("Matrix created\n");
}

void Matrix::set_matrix(char **newGrid) {
	grid = newGrid;		
}

void Matrix::set_matrix(char *str) {
	int i = 0;
	for (int row = 0; row < rows; row++) {
		for (int col = 0; col < colums; col++) {
			grid[row][col] = str[i];
			i++;
		}
	}
}

void Matrix::set_rim(char *rim) {
	strcpy(matrixRim, rim);
}

char *Matrix::get_rim() {
	return matrixRim;
}

char **Matrix::get_matrix() {
	return grid;	
}

void Matrix::print_matrix() {
	for (int row = 0; row < rows; row++) {
		for (int col = 0; col < colums; col++)
			printf("%c", grid[row][col]);
		printf("\n");
	}
}

std::string Matrix::to_string() {
	std::string str_matrix;
	for (int row = 0; row < rows; row++) {
		for (int col = 0; col < colums; col++)
			str_matrix += grid[row][col];
	}
	return str_matrix;
}

int Matrix::get_rows() {
	return rows;
}

int Matrix::get_colums() {
	return colums;
}

void Matrix::set_rows(int rows) {
	Matrix::rows = rows;
}

void Matrix::set_colums(int colums) {
	Matrix::colums = colums;
}
