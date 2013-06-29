#pragma once

const char* timeString();

unsigned int fileSize(const char *name);
void readFile(const char *name, char* buff);

string format(const char* formatString, ...);

// forward declaration
class GLPresenter;
