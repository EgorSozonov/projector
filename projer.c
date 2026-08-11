//{{{ Includes

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

typedef int32_t Int;
typedef uint32_t Unt;
typedef int64_t Long;
typedef uint64_t Ulong;
typedef char Byte;
typedef bool Bool;
#define private static
#define Arr(T) T*
#define null NULL
#define ei else if
#define OUT
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))


jmp_buf excBuf;

#define print(...) \
  printf(__VA_ARGS__);\
  printf("\n");

//}}}
//{{{ Utils
//{{{ Arena

#define CHUNK_QUANT 32768

typedef struct ArenaChunk ArenaChunk;

struct ArenaChunk { // :ArenaChunk
   size_t size;
   ArenaChunk* next;
   char memory[]; // flexible array member
};

typedef struct { // :Arena
   ArenaChunk* firstChunk;
   ArenaChunk* currChunk;
   int currInd;
} Arena;


private size_t
calculateChunkSize(size_t allocSize) { //:calculateChunkSize
// Calculates memory for a new chunk. Memory is quantized and is always 32 bytes less
// 32 for any possible padding malloc might use internally,
// so that the total allocation size is a good even number of OS memory pages
   size_t fullMemory = sizeof(ArenaChunk) + allocSize + 32;
   // struct header + main memory chunk + space for malloc bookkeep

   int mallocMemory = fullMemory < CHUNK_QUANT
                  ? CHUNK_QUANT
                  : (fullMemory % CHUNK_QUANT > 0
                     ? (fullMemory/CHUNK_QUANT + 1)*CHUNK_QUANT
                     : fullMemory);

   return mallocMemory - 32;
}

void*
allocateOnArena(size_t allocSize, Arena* a) { //:allocateOnArena
// Allocate memory in the arena, malloc'ing a new chunk if needed
   if ((size_t)a->currInd + allocSize >= a->currChunk->size) {
      if (a->currChunk->next != null && a->currChunk->next->size < allocSize) {
         // the next chunk is big enough, so we skip the rest of this chunk and move on
         print("reusing cleared memory from the arena!")
         a->currChunk = a->currChunk->next;
         a->currInd = 0;
      } else { // we need to allocate new chunk

         size_t newSize = calculateChunkSize(allocSize);
         ArenaChunk* newChunk = malloc(newSize);
         if (!newChunk) {
            perror("malloc error when allocating arena chunk");
            exit(EXIT_FAILURE);
         };
         // sizeof counts everything but the flexible array member, that's why we subtract it
         newChunk->size = newSize - sizeof(ArenaChunk);
         newChunk->next = a->currChunk->next; // if the arena has a (small) tail, don't lose it

         a->currChunk->next = newChunk;
         a->currChunk = newChunk;
         a->currInd = 0;
      }

   }
   void* result = (void*)(a->currChunk->memory + (a->currInd));
   a->currInd += allocSize;
   if (allocSize % 4 != 0)  {
      a->currInd += (4 - (allocSize % 4));
   }
   return result;
}

void
deleteArena(Arena* ar) { //:deleteArena
// Returns memory of the arena to the OS
   ArenaChunk* curr = ar->firstChunk;
   while (curr != null) {
      ArenaChunk* nextToFree = curr->next;
      free(curr);
      curr = nextToFree;
   }
   free(ar);
}

private void
clearArena(Arena* a) { //:clearArena
// Clears the memory of the arena for reuse. Does not free memory.
   a->currChunk = a->firstChunk;
   a->currInd = 0;
}

#define CHUNK_QUANT 32768

typedef struct ArenaChunk ArenaChunk;

struct ArenaChunk { // :ArenaChunk
   size_t size;
   ArenaChunk* next;
   char memory[]; // flexible array member
};

struct Arena { // :Arena
   ArenaChunk* firstChunk;
   ArenaChunk* currChunk;
   int currInd;
};


private size_t
minChunkSize(void) {
   return (size_t)(CHUNK_QUANT - 32);
}

Arena*
createArena() { //:createArena
   Arena* result = malloc(sizeof(Arena));

   size_t firstChunkSize = minChunkSize();
   ArenaChunk* firstChunk = malloc(firstChunkSize);
   if (!result || !firstChunk)
      { longjmp(excBuf, 1); }

   firstChunk->size = firstChunkSize - sizeof(ArenaChunk);
   firstChunk->next = null;
   result->firstChunk = firstChunk;
   result->currChunk = firstChunk;
   result->currInd = 0;
   return result;
}

#define allocate(T, a) (T*)allocateOnArena(sizeof(T), a)
#define allocateArray(cap, T, a) (T*)allocateOnArena(cap*sizeof(T), a)
#define containerOf(ptr, Type, member) ((Type *)((char *)(ptr) - offsetof(Type, member)))

//}}}
//{{{ List

#define DEFINE_LIST_HEADER(T) \
   typedef struct {\
      T* c;\
      Int len;\
      Int cap;\
      Arena* arena;\
   } L##T;\
   private L ## T * createL ## T (Int initCapacity, Arena* a);\
   private T removeLast ## T (L##T * st);\
   private void add ## T (T newItem, L##T * st);

#define DEFINE_LIST(T)\
   private L##T * createL##T (int initCapacity, Arena* a) {\
      int capacity = initCapacity < 4 ? 4 : initCapacity;\
      L##T * result = allocate(L##T, a);\
      result->cap = capacity;\
      result->len = 0;\
      result->arena = a;\
      T* arr = allocateArray(capacity, T, a);\
      result->c = arr;\
      return result;\
   }\
   private T removeLast##T (L##T * st) {\
      st->len--;\
      return st->c[st->len];\
   }\
   private void add##T (T newItem, L##T * st) {\
      if (st->len < st->cap) {\
         memcpy((T*)(st->c) + (st->len), &newItem, sizeof(T));\
      } else {\
         T* newcent = allocateArray(2*(st->cap), T, st->arena);\
         memcpy(newcent, st->c, st->len*sizeof(T));\
         memcpy((T*)(newcent) + (st->len), &newItem, sizeof(T));\
         st->cap *= 2;\
         st->c = newcent;\
      }\
      st->len++;\
   }\

#define last(lst) lst->c[lst->len - 1]

DEFINE_LIST_HEADER(Int)
DEFINE_LIST_HEADER(Unt)
DEFINE_LIST_HEADER(Ulong)
DEFINE_LIST_HEADER(Byte);
DEFINE_LIST(Int);
DEFINE_LIST(Unt);
DEFINE_LIST(Ulong);
DEFINE_LIST(Byte);

//}}}
//{{{ Strings

typedef struct { // :Text
    char const* c;
    int32_t len;
} Text;

typedef struct { // :StringBuilder
   Arr(char) c;
   Int len;
   Int cap;
   Arena* a;
} StringBuilder;

private StringBuilder //:createStringBuilder
createStringBuilder(Arena* a) {
   return (StringBuilder){.c = allocateArray(4, Byte, a), .len = 0, .cap = 4, .a = a};
}

private Bool //:equal
equal(Text a, Text b) {
   if (a.len != b.len) {
      return false;
   }

   int cmpResult = memcmp(a.c, b.c, b.len);
   return cmpResult == 0;
}

private Text //:text
text(char const* cString) {
   Int len = strlen(cString);
   return (Text){.c = cString, .len = len};
}

constexpr Text empty = {.c = null, .len = 0};

void //:printString
printString(Text s) {
   if (s.len == 0)
      { return; }
   fwrite(s.c, 1, s.len, stdout);
   printf("\n");
}

void //:printStringNoLn
printStringNoLn(Text s) {
   if (s.len == 0)
      { return; }
   fwrite(s.c, 1, s.len, stdout);
}

private void //:ensureCapacityStringBuilder
ensureCapacityStringBuilder(Int neededSpace, StringBuilder* sb) {
   if (neededSpace + sb->len < sb->cap)
      { return; }
   Int newCap = MAX(sb->len + neededSpace, 2*sb->len);
   Arr(Byte) newContent = allocateArray(newCap, Byte, sb->a);
   memcpy(newContent, sb->c, sb->len);
   sb->cap = newCap;
   sb->c = newContent;
}

private void //:appendStringBuilder
appendStringBuilder(Text a, StringBuilder* sb) {
   ensureCapacityStringBuilder(a.len, sb);
   memcpy(sb->c + sb->len, a.c, a.len);
   sb->len += a.len;
}


private void //:charAppendStringBuilder
charAppendStringBuilder(char c, StringBuilder* sb) {
   ensureCapacityStringBuilder(1, sb);
   sb->c[sb->len] = c;
   sb->len++;
}

private void //:printStringBuilder
printStringBuilder(StringBuilder s) {
   if (s.len == 0)
      { return; }
   fwrite(s.c, 1, s.len, stdout);
   printf("\n");
}

private Text //:freezeStringBuilder
freezeStringBuilder(StringBuilder* sb) {
   ensureCapacityStringBuilder(1, sb);
   sb->c[sb->len] = '\0';
   return (Text){.c = sb->c, .len = sb->len};
}

private Bool //:containsChar
containsChar(Text s, char c) {
   for (Int j = 0; j < s.len; j++) {
      if (s.c[j] == c)
      { return true; }
   }
   return false;
}

private Text
concat(char const* s1, char const* s2, Arena* a) {
   Int len1 = strlen(s1);
   Int len2 = strlen(s2);
   Arr(char) concatenated = allocateOnArena(len1 + len2 + 1, a);
   memcpy(concatenated, s1, len1);
   memcpy(concatenated + len1, s2, len2);
   concatenated[len1 + len2] = '\0';
   return (Text){.c = concatenated, .len = len1 + len2};
}

private Text //:concatDirAndSub
concatDirAndSub(Text dir, char const* sub, Arena* a) {
   Int len2 = strlen(sub);
   Int totalLen = dir.len + len2 + 1;
   Arr(char) concatenated = allocateOnArena(totalLen + 1, a);
   memcpy(concatenated, dir.c, dir.len);
   concatenated[dir.len] = '/';
   memcpy(concatenated + dir.len + 1, sub, len2);
   concatenated[totalLen] = '\0';
   return (Text){.c = concatenated, .len = totalLen};
}

private Int //:skipSpaces
skipSpaces(Arr(char const) source, Int k, Int sentinel) {
   Int l = k;
   for (; l < sentinel; l++) {
      Byte currBt = source[l];
      if (currBt != ' ')
         { return l; }
   }
   return l;
}

//}}}
//{{{ Line utils

void
findLineBounds(Text s, Int startInd, OUT Int* lineStart, OUT Int* lineEnd) {
   Int j;
   for (j = startInd; j < s.len && s.c[j] != '\n'; j++) {
   }
   *lineEnd = j;
   for (j = startInd; j > -1 && s.c[j] != '\n'; j--) {
   }
   *lineStart = j + 1;
}

Bool
compareResults(Text programOutput, Text expectedResult) {
   if (programOutput.len != expectedResult.len) {
      return false;
   } ei (programOutput.len == 0) {
      return true;
   }
   return memcmp(programOutput.c, expectedResult.c, programOutput.len) == 0;
}

//}}}
//{{{ File system utils

private Bool //:shouldDirBePrinted
shouldDirBePrinted(char* name, Text fullName) {
   struct stat stBuf;
   if (name[0] == '.' && (!name[1] || (name[1] == '.' && !name[2])))
      { return false; }
   stat(fullName.c, &stBuf);
   return S_ISDIR(stBuf.st_mode) > 0;
}

private void //:printSubDirs
printSubDirs(Text configDir) {
   DIR *dir = opendir(configDir.c);
   Arena* a = createArena();

   for (struct dirent *ent = readdir(dir); ent; ent = readdir(dir)) {
      char* entryName = ent->d_name;
      Text fullName = concatDirAndSub(configDir, entryName, a);
      if (shouldDirBePrinted(entryName, fullName)) {
         printf("%s\n", entryName);
      }
   }
   closedir(dir);
   deleteArena(a);
}

private Bool
shouldTemplateBePrinted(char* name, Text fullName) {
   struct stat stBuf;
   stat(fullName.c, &stBuf);
   return S_ISDIR(stBuf.st_mode) == 0;
}

private void //:printTemplates
printTemplates(Text configDir, char* subdir) {
   Arena* a = createArena();
   Text dirn = concatDirAndSub(configDir, subdir, a);
   DIR *dir = opendir(dirn.c);
   if (!dir) {
      print("Subdir %s doesn't exist in ~/.config/projector", subdir);
      return;
   }
   
   print("Available templates:");
   printf("\n");
   for (struct dirent *ent = readdir(dir); ent; ent = readdir(dir)) {
      char* entryName = ent->d_name;
      Text fullName = concatDirAndSub(dirn, entryName, a);
      if (shouldTemplateBePrinted(entryName, fullName)) {
         printf("%s\n", entryName);
      }
   }
   closedir(dir);
   deleteArena(a);
}

private void //:mbCreateDir
mbCreateDir(char const* path) {
   struct stat st = {0};

   if (stat(path, &st) == -1) {
       mkdir(path, S_IRUSR | S_IWUSR | S_IXUSR);
   }
}

//}}}

#define add(A, X) _Generic((X),\
   LInt*: addInt,\
   LUnt*: addUnt,\
   LUlong*: addUlong,\
   LCreateFile*: addCreateFile,\
   LVar*: addVar\
)(A, X)

//}}}
//{{{ Types

typedef struct { //:Var
   Text name;
   Text value;
} Var;

typedef struct { //:CreateFile
   Text relativeName;
   Text content; // with vars not substituted yet
} CreateFile;

DEFINE_LIST_HEADER(Var)
DEFINE_LIST_HEADER(CreateFile)
DEFINE_LIST(Var)
DEFINE_LIST(CreateFile)


typedef enum { //:WhatToDo
   whatToDoPrintHelp,
   whatToDoPrintTemplateDirs,
   whatToDoPrintTemplates,
   whatToDoInitProject
} WhatToDo;

typedef struct { //:TaskDescription
   Text inputFilename;
   WhatToDo whatToDo;
   Text configDir;
   Text projName;
   Text errMsg;
   LVar* vars;
} TaskDescription;


typedef struct { //:Project
   Text name;
   LVar* vars;
   LCreateFile* files;
   Text errMsg;
} Project;

#define aALower       97
#define aZLower      122
#define aAUpper       65
#define aZUpper       90
#define aDigit0       48
#define aDigit9       57

//}}}
//{{{ Building projects

private void //:createAFile
createAFile(CreateFile cf) {

}

private StringBuilder //:buildFullFileName
buildFullFileName(CreateFile cf, Project proj, Arena* a) {
   StringBuilder sb = createStringBuilder(a);
   appendStringBuilder(proj.name, &sb);
   charAppendStringBuilder('/', &sb);
   appendStringBuilder(cf.relativeName, &sb);
   return sb;
}

private void //:createIntermittentDirs
createIntermittentDirs(StringBuilder fullName, Text projName) {
// For full name `projectName/foo/bar/file.c`, creates subdirs `foo` and `foo/bar`
   Int j = projName.len + 1;
   for (; j < fullName.len; j++) {
      if (fullName.c[j] == '/') {
         fullName.c[j] = '\0';
         mbCreateDir(fullName.c);
         fullName.c[j] = '/';
      }
   }
}

private void //:initProject
initProject(Project proj, Arena* a) {
// The main function. Actually creates the necessary files and folders for a project
   mbCreateDir(proj.name.c);
   for (Int j = 0; j < proj.files->len; j++) {
      CreateFile toCreate = proj.files->c[j];
      StringBuilder fullFileName = buildFullFileName(toCreate, proj, a);

      createIntermittentDirs(fullFileName, proj.name);
      
      FILE* newFile = fopen(fullFileName.c, "w");
      if (newFile == null) {
         print("Error opening file for writing:");
         printStringBuilder(fullFileName);
         return;
      }
      fwrite(toCreate.content.c, 1, toCreate.content.len, newFile);
      fflush(newFile);
      fclose(newFile);
   }
}

//}}}
//{{{ Template parsing

#define BUFFER_SIZE 1024
extern jmp_buf excBuf;

private constexpr Text
opener = (Text){.c = "////{", .len = 5};
private constexpr Text
closer = (Text){.c = "////}", .len = 5};

LByte* buildFilename(Text programName, Arena* a);

private Text //:openTemplate
openTemplate(Text fName, Arena* a) {
   FILE *template = fopen(fName.c, "r");
   if (!template) {
      print("Template file not found!");
      print("%s", fName);
      return empty;
   }

   // Go to the end of the file
   if (fseek(template, 0L, SEEK_END) != 0)
      { goto cleanup; }
   long fileSize = ftell(template);
   if (fileSize == -1)
      { goto cleanup; }
   // Allocate our buffer to that size, with space for the standard text in front of it
   Arr(char) result = allocateOnArena(fileSize + 1, a);

   // Go back to the start of the file
   if (fseek(template, 0L, SEEK_SET) != 0)
      { goto cleanup; }

   Int const len = fread(result, 1, fileSize, template);
   if (ferror(template) != 0 ) {
      fputs("Error reading file", stderr);
   } else {
      result[len] = '\0';
   }
   cleanup:
   fclose(template);
   return (Text){.c = result, .len = len};
}

[[noreturn]] private void //:pError
pError(char const* msg, Int line) {
   print("Line %d", line);
   print("%s", msg);
   longjmp(excBuf, 1);
}

private Bool //:isAlphanumeric
isAlphanumeric(char a) {
   return (a >= aALower && a <= aZLower)
         || (a >= aAUpper && a <= aZUpper)
         || (a >= aDigit0 && a <= aDigit9);
}

private Text //:substituteVars
substituteVars(char const* text, Int len, LVar* restrict vars, Arena* a) {
// Substitutes all known vars in a character array, and produces a new string
// %%var -> substitution
   char const* prev = text;
   char const* curr = text;
   char const* const sentinel = text + len;
   StringBuilder result = (StringBuilder){
      .c = allocateArray(len, Byte, a), .len = 0, .cap = len, .a = a
   };

   for (; curr < sentinel; curr++) {
      if (*curr != '%' || (curr + 1) == sentinel || *(curr + 1) != '%')
         { continue; }
      char const* wordEnd = curr + 2;
      for (; wordEnd < sentinel && isAlphanumeric(*wordEnd); wordEnd++) {}
      if (wordEnd - curr == 2) // just a single %%
         { continue; }

      Text varInTemplate = (Text){.c = curr + 2, .len = wordEnd - curr - 2}; // +-2 for the `%%`
      Bool foundTheVar = false;
      for (Int j = 0; j < vars->len; j++) {
         Var var = vars->c[j];
         if (var.name.len == varInTemplate.len
               && memcmp(var.name.c, varInTemplate.c, var.name.len) == 0
         ) {
            Int const lenAddition = curr - prev + var.value.len;
            ensureCapacityStringBuilder(lenAddition, &result);
            memcpy(result.c + result.len, prev, curr - prev);
            memcpy(result.c + result.len + (curr - prev), var.value.c, var.value.len);
            result.len += lenAddition;

            curr += (var.name.len + 2); // 2 for the `%%`
            prev = curr;
            foundTheVar = true;
            break;
         }
      }
      if (!foundTheVar) {
         printf("Value for variable not provided on the command line: ");
         printString(varInTemplate);
         longjmp(excBuf, 1);
      }
   }
   if (prev < curr) {
      Int const lenAddition = curr - prev;
      ensureCapacityStringBuilder(lenAddition, &result);
      memcpy(result.c + result.len, prev, curr - prev);
      result.len += lenAddition;
   }
   return freezeStringBuilder(&result);
}

private Project //:parseTemplate
parseTemplate(Text template, Text projName, LVar* vars, Arena* a) {
   Project project = (Project){.files = createLCreateFile(2, a), .vars = vars, .name = projName
   };

   Int i = 0; // line start
   Int k = 0; // line end
   Int line = 0;
   Bool insideFile = false;
   CreateFile currFile;
   for (; i < template.len; k++, i = k, line++) {
      for (; k < template.len && template.c[k] != '\n'; k++) {
      }
      Int const lineLen = k - i;
      if (lineLen >= opener.len && memcmp(template.c + i, opener.c, opener.len) == 0) {
         if (insideFile) {
            pError("Error: file opener (`////{`) but the previous file hasn't been closed", line);
         }

         Int j = skipSpaces(template.c, i + opener.len, k);
         if (j == k)
            { pError("Error: file name empty", line); }

         currFile.relativeName = (Text){.c = template.c + j, .len = k - j};
         currFile.content = (Text){.c = template.c + k + 1}; // +1 to skip the newline after name
         insideFile = true;
         continue;
      } ei (lineLen >= closer.len && memcmp(template.c + i, closer.c, closer.len) == 0) {
         if (!insideFile) {
            print("Line %d", line);
            print("Error: file closer (`////}`) but we are not inside a file");
            longjmp(excBuf, 1);
         }

         Int fileLen = i - 1 - (currFile.content.c - template.c);
         currFile.relativeName =
            substituteVars(currFile.relativeName.c, currFile.relativeName.len, project.vars, a);
         currFile.content = substituteVars(currFile.content.c, fileLen, project.vars, a);

         add(currFile, project.files);
         insideFile = false;
         continue;
      }
   }
   return project;
}

void //:tryReadTemplate
tryReadTemplate(TaskDescription task) {
   Arena* a = createArena();
   Int len = task.configDir.len + task.inputFilename.len + 6;
   char* fName = allocateOnArena(len, a);
   memcpy(fName, task.configDir.c, task.configDir.len);
   fName[task.configDir.len] = '/';
   memcpy(fName + task.configDir.len + 1, task.inputFilename.c, task.inputFilename.len);
   memcpy(fName + task.configDir.len + task.inputFilename.len + 1, ".proj", 5);
   fName[len] = '\0';

   Text fileContents = openTemplate((Text){.c = fName, .len = len}, a);
   if (fileContents.len == 0)
      { goto cleanup; }

   Project project = parseTemplate(fileContents, task.projName, task.vars, a);
   if (project.errMsg.len > 0) {
      printString(project.errMsg);
      goto cleanup;
   }

   initProject(project, a);

   cleanup:
   deleteArena(a);
}

//}}}
//{{{ Main

Text errMsg = empty;

//{{{ Command line

private void //:printHelp
printHelp() {
   print("Tool to initialize projects from templates. Usage:\n\n"
      "> projector --name project java/basic\n\n"
      "Creates a project in folder \"./project\" according to template in ~/.config/java/basic.proj"
      "\n\nOther options:\n"
      "-h    print this help\n"
   );
}

private Var //:getProjectName
getProjectName(LVar* vars) {
   for (Int j = 0; j < vars->len; j++) {
      Var v = vars->c[j];
      if (v.name.len == 4 && equal(v.name, text("name"))) {
         return v;
      }
   }
   errMsg = text("The project name must be in the NAME environment var");
   longjmp(excBuf, 1);
}

private Var //:commandLineVar
commandLineVar(Int argInd, Int argc, char** argv, Arena* a) {
// A `--name=foo` expression
   char const* arg = argv[argInd];
   Int j = 2;
   for (; isAlphanumeric(arg[j]); j++) {}
   if (arg[j] == '=') {
      j++;
      Int valueStart = j;
      for (; arg[j] != '\0'; j++) {}
      Int len = j - valueStart;
      if (len == 0) {
         errMsg = text("Empty value of command argument");
         longjmp(excBuf, 1);
      }
      return (Var){
         .name = (Text){.c = arg + 2, .len = valueStart - 3 },
         .value = (Text){.c = arg + valueStart, .len = j - valueStart }
      };
   } else {
      errMsg = text("Command line arguments must look like `--name=foo`");
      longjmp(excBuf, 1);
   }
}

private TaskDescription //:getCommandParams
getCommandParams(int argc, char** argv, Arena* a) {
   if (argc == 1) { // no arguments to the program
      return (TaskDescription){
            .inputFilename = empty, .vars = null,
            .errMsg = empty, .whatToDo = whatToDoPrintTemplateDirs
      };
   }
   Text inputFilename = empty;
   TaskDescription task = (TaskDescription){
      .vars = createLVar(2, a), .inputFilename = inputFilename, .whatToDo = whatToDoInitProject
   };
   for (Int j = 1; j < argc; j++) {
      char* argCString = argv[j];
      Text arg = text(argCString);
      if (arg.c[0] == '-') {
         if (arg.len == 1) {
            errMsg = text("Empty option");
            goto finish;
         } ei (arg.len == 2) {
            if (arg.c[1] == 'h') {
               task.whatToDo = whatToDoPrintHelp;
               goto finish;
            }
         } ei (arg.c[1] == '-') {
            add(commandLineVar(j, argc, argv, a), task.vars);
            continue;
         }
         errMsg = text("Unknown option");
         goto finish;
      } else {
         if (inputFilename.len == 0) {
            inputFilename = arg;
         } else {
            errMsg = text("Only 1 input file is allowed");
            goto finish;
         }
      }
   }
   if (containsChar(inputFilename, '/')) {
      task.whatToDo = whatToDoInitProject;
      task.projName = getProjectName(task.vars).value;
   } ei (inputFilename.len > 0) {
      task.inputFilename = inputFilename;
      task.whatToDo = whatToDoPrintTemplates;
   } else {
      task.whatToDo = whatToDoPrintTemplateDirs;
   }
finish:
   task.errMsg = errMsg;
   return task;
}

void
printError(Text e) {
   printf("Error: ");
   printString(e);
}

//}}}

private Text
getConfigDir(Arena* a) {
   char const* homeDir = getenv("HOME");
   Int homeDirLen = strlen(homeDir);
   Int configDirLen = homeDirLen + 18; // 18 = len of `/.config/projector`
   char* const configDir = allocateOnArena(configDirLen + 1, a);
   memcpy(configDir, homeDir, homeDirLen);
   memcpy(configDir + homeDirLen, "/.config/projector", 18);
   configDir[configDirLen] = '\0';
   return (Text){.c = configDir, .len = configDirLen};
}

void run(TaskDescription task) {
   switch (task.whatToDo) {
   case (whatToDoPrintHelp): {
      printHelp();
      break;
   }
   case (whatToDoPrintTemplates): {
      printTemplates(task.configDir, task.inputFilename.c);
      break;
   }
   case (whatToDoPrintTemplateDirs): {
      print("Available template directories in ~/.config/projector:");
      printf("\n");
      printSubDirs(task.configDir);
      print("\nOr run projector -h to get help");
      break;
   }
   case (whatToDoInitProject): {
      tryReadTemplate(task);
      print("Project scaffolding generated");
      break;
   }
   }
}

int main(int argc, char** argv) {
   Arena* a = createArena();
   if (setjmp(excBuf) == 0) {

      TaskDescription task = getCommandParams(argc, argv, a);
      task.configDir = getConfigDir(a);
      if (task.errMsg.len > 0) {
         printError(task.errMsg);
         return 0;
      }

      mbCreateDir(task.configDir.c);
      run(task);
   } else {
      if (errMsg.len > 0)
         { printError(errMsg); }
   }

   cleanup:
   deleteArena(a);
   return 0;
}

//}}}
