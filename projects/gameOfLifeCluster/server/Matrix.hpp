#include <iostream>
#include <stdio.h>
#include <string.h>
#include <vector>
class Matrix {

	public:
		Matrix(int width, int height);
		Matrix();
		void printMatrix();
		std::string toString();
		char** getMatrix();
		char* getRim();
		int getRows();
		int getColums();
        void setRows(int rows);
        void setColums(int colums);
		void setMatrix(char** newGrid);
		void setMatrix(char* str);
		void setRim(char* rim);
	private:
		int rows;
		int colums;
		char** grid;
		char* matrixRim;
};
