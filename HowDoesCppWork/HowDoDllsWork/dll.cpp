#include <cstdio>

void ANestedDllFunction();

__declspec(dllexport) void MySpecialFunctionThatLivesInADll()
{
	printf("Hey! I live in an DLL!\n");
	printf("Actually, I basically am just like those other functions.\n");
	printf("With a few exceptions, of course ;)\n");

	printf("And now, I'm going to call my friend!\n\n");

	ANestedDllFunction();
}