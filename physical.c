#include <time.h>
#include <stdlib.h>

int rate_drop;
int rate_corrupt;

int initialize_physic()
{
  srand(time(NULL));
}

int send(char* frame)
{
  int drop = rand() % 100 <= rate_drop;
  if(drop)
    return 0;
  
  int corrupt = rand() % 100 <= rate_corrupt;
  if(corrupt)
  {
    frame[rand() % strlen(frame) += 1;
  }

      //      ..
}
