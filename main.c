//
//  main.c
//  dskcrcfixer
//
//  Created by Thomas Harte on 02/01/2017.
//  Copyright © 2017 Thomas Harte. All rights reserved.
//

#include <stdio.h>
#include <string.h>

int main(int argc, const char * argv[]) {
	if(argc < 2)
	{
		const char *app_name = argv[0] + strlen(argv[0]);
		while(app_name > argv[0] && app_name[0] != '/') app_name--;
		printf("Usage: %s [name of dsk]\n", &app_name[1]);
		return -1;
	}
	printf("Hello, World!\n");
    return 0;
}
