/************************** SVN Revision Information **************************
 **    $Id$    **
******************************************************************************/
 
int 	NPES_part1;
int 	NPES_part2;

int 	MXLLDA_part1; 
int 	MXLLDA_part2; 


struct BOX {

	int 	x1, x2, y1, y2, z1, z2; 
}; 
typedef	struct 	BOX 	BOX; 

struct COMPASS {

	int 	type; 
	BOX  	box1, box2, box3, box4;  
}; 
typedef	struct	COMPASS	COMPASS;


COMPASS 	chargeDensityCompass; 
COMPASS 	potentialCompass; 
