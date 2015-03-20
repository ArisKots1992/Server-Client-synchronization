#include "header.h"

/* Creating a static 2Darray_Tree from filename */

struct Encoded** TreeCreation(int height, int fan_out, char* filename){

struct Encoded **level;
	level = new struct Encoded*[ height ];
	for( int i=0; i<height; i++)
		level[i] = new struct Encoded[ power(fan_out,i) ];



FILE * fp;
char* buffer = new char[ PAGESIZE ];
size_t read;

	fp = fopen(filename, "rb");               //open the file for binary input
	if( fp == NULL ){
        	perror("Could not open file");
		exit(1);
	}

int page=0;
	do{
		read = fread(buffer, 1, PAGESIZE, fp);

		if( read > 0 ){
			if( read < PAGESIZE){					//if fewer bytes than requested were returned.
				 memset(buffer + read , '0', PAGESIZE - read);	//fill the remainder of the buffer with zeroes
			}

                	MD5_CTX context;
                	MD5Init( &context );
                	MD5Update( &context , buffer , PAGESIZE );
                	MD5Final(  level[ height-1 ][ page ].digest , &context );	//fill the leaves with pages digests
			page++;
		}
	}while(read == PAGESIZE);		//end when a read returned fewer items


	while( page < power(fan_out,height-1) ){				// fill the rest with blank pages hash
		                        MD5_CTX context;
		MD5Init( &context );
		MD5Update( &context , "" , 0 );					//the blank
		MD5Final(  level[ height-1 ][ page ].digest , &context );       //fill the leaves with blank pages
		page++;
	}
        int lvl = height -2;                    //now i concat until the root so i start from the nodes above leaves


	while( lvl >= 0){
		for(int i=0; i<power(fan_out,lvl); i++){
			MD5_CTX context;
			MD5Init( &context );
			for(int j=fan_out*i; j<fan_out*(i+1); j++)
				MD5Update( &context ,(char*) (level[lvl+1][j].digest) , 4 * sizeof( int ) );
			MD5Final( level[lvl][i].digest , &context );
		}
		lvl--;
	}

fclose(fp);
return level;

}

/* Only allocating memory for the new Tree */

struct Encoded** TreeAllocation(int height, int fan_out){

struct Encoded **level;
        level = new struct Encoded*[ height ];
        for( int i=0; i<height; i++)
                level[i] = new struct Encoded[ power(fan_out,i) ];


return level;
}


int power(int a, int b)
{
	if( b == 0)
		return 1;
	int c=a;
	for (int n=b; n>1; n--) c*=a;
	return c;
}

/* Transform 2D to 1D but simultaniusly creating the 1D array */

void Transform2D_to_1D(int height,int fan_out,struct Encoded**& level,struct Encoded*& new_level){

	int nodes =( power(fan_out, height)-1)/(fan_out-1);

	new_level = new struct Encoded[nodes];

	int h=0;
	int counter=0;

	while( h < height){
		for(int i=0;i<power(fan_out,h);i++){
			for(int j=0;j<4;j++)
				new_level[counter].digest[j] = level[h][i].digest[j];
			counter++;
		}
		h++;
	}
}

/* filling the already existing 2D array with the 1D recieved Data */

void Assign1D_to_2D(int height, int fan_out, struct Encoded**& level, struct Encoded*& new_level){

        int h=0;
        int counter=0;

        while( h < height){
                for( int i=0; i<power(fan_out,h); i++ ){
                        for( int j=0; j<4; j++ )
				level[h][i].digest[j] = new_level[counter].digest[j];
                        counter++;
                }
                h++;
        }
}
