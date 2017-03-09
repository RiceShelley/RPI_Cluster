#include <iostream>
#include <stdio.h>
#include <string.h>
#include <vector>

class Matrix {
	public:
		Matrix(int width, int height);
		Matrix();
		void print_matrix();
		std::string to_string();
		char **get_matrix();
		char *get_rim();
		int get_rows();
		int get_colums();
		void set_rows(int rows);
		void set_colums(int colums);
		void set_matrix(char **newGrid);
		void set_matrix(char *str);
		void set_rim(char *rim);
	private:
		int rows;
		int colums;
		char **grid;
		char *matrixRim;
};
