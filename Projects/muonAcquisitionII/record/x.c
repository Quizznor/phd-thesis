#include <stdio.h>
#include <sys/stat.h>
struct b
{

};
int main()
{
  struct b bb;
  struct stat st;
  if(stat("a",&st)==0){
    printf("File exist");
  } else {
    printf("Not File exist");
  }
  return(0);
}
