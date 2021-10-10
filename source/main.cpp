#include"solver.h"
#include"window.h"

// Domain size / listening position constants
// These should be set based on the models/user input later
#define DOMAIN_SIZE_X 1
#define DOMAIN_SIZE_Y 1
#define CELL_SIZE 0.002f

#define LISTENING_X 0.5f
#define LISTENING_Y 0.5f


int main(){

	Solver solver(DOMAIN_SIZE_X, DOMAIN_SIZE_Y, CELL_SIZE, LISTENING_X, LISTENING_Y);

	Window window(solver);
	window.init();
	window.run();
}
