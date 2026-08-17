
# Projer

Purpose: a simple program to generate project scaffolding from text templates.

### Building

```
make all
sudo make install
```

### Example usage

Write a template like this:

```
////{ src/%%name.c

#include <stdio.h>

int main(int argc, char** argv){
   printf("HW\n");
}

////}
```

And place it in the file ~/.config/projer/c/simple.proj

Go into a folder that you'd like to create a project in.

Call the program like

`projer --name=new c/simple`

It will create the folder "new" and inside it the file "src/new.c" with the contents from the 
template.

Variables are passed on the command line like

`projer --name=foo --var1=a --var2=b ...`

and interpolated in both filenames and contents of the templated files. The "name" variable is
mandatory and will be the name of the directory created in the current dir.

That's all there is to it! Enjoy!
