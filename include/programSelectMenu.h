
void program_select_func ( int k )
{
	switch (k)
	{
	case 1:	system("START Part1_Icosahedron.exe");	exit(0);	break;
	case 2:	system("START Part2_TychoBrahe.exe");	exit(0);	break;
	case 3:	system("START Part3_ModelLoader.exe");	exit(0);	break;
	}
}

GLuint init_program_select_menu ( void )
{
	// Create right-click menu:
	GLuint menid = glutCreateMenu(program_select_func);
	glutAddMenuEntry("Part I   : Icosahedron", 1);
	glutAddMenuEntry("Part II  : Tycho Brahe", 2);
	glutAddMenuEntry("Part III : OBJ Model Loader", 3);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	return menid;
}
