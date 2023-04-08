#include <string>
#include <stdexcept>
#include <cstdio>

#define BUFFER 2048

std::string runUnixCommandAndCaptureOutput(std::string cmd) {
   char buffer[BUFFER];
   std::string result = "";
   FILE* pipe = popen(cmd.c_str(), "r");
   if (!pipe) throw std::runtime_error("popen() failed!");
   try {
      while (!feof(pipe)) {
         if (fgets(buffer, BUFFER, pipe) != NULL)
            result += buffer;
      }
   } catch (...) {
      pclose(pipe);
      throw;
   }
   pclose(pipe);
   return result;
}
