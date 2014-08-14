// Copyright 2012 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <unistd.h>
#include <cstring>
#include <queue>
#include "v8.h"


using namespace v8;


/** Read a file into a v8 string. */
Handle<String> ReadFile(v8::Isolate* isolate, char const* file_name)
{
    FILE* file = fopen(file_name, "rb");
    if (file == NULL)
        return v8::Handle<v8::String>();
 
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    rewind(file);
 
    char* chars = new char[size + 1];
    chars[size] = '\0';
    for (int i = 0; i < size;)
    {
        int read = static_cast<int>(fread(&chars[i], 1, size - i, file));
        i += read;
    }
 
    fclose(file);
    Handle<String> result =
        String::NewFromUtf8(isolate, chars, String::kNormalString, size);
    delete[] chars;
    return result;
}


/** Send message out of the sandbox. */
void WriteMessage (int const& fd, char const* message)
{
    char const newline = '\n';

    write (fd, message, strlen(message));
    write (fd, &newline, 1);
}


/** Read message from outside the sandbox.
      Return message as a V8 string.
*/
void ReadMessage (Isolate* isolate, int const& fd, Handle<String> *message)
{
    char buf[1024];
    size_t bytes_read;

    if (message==NULL)
        message = new Handle<String>(String::NewFromUtf8(isolate, ""));
    else
        *message = String::NewFromUtf8(isolate, "");

    do
    {
        bytes_read = read(fd, buf, sizeof(buf));        
        *message = String::Concat (*message, String::NewFromUtf8(isolate, buf, String::kNormalString, bytes_read));
    } while (bytes_read==sizeof(buf));
}


/** Read messages from outside the sandbox.
      This is the event loop.
      It handles all asynchronous responses.
      Pass all messages to 'onmessage' defined in the contract.
*/
void *ReadMessages (Isolate *isolate)
{
    Handle<Value> args[1];
    Handle<String> message;
    Handle<Value> value = isolate->GetCurrentContext()->Global()->Get(String::NewFromUtf8(isolate, "onmessage"));

    if (value->IsFunction())
    {
        Handle<Function> func = Handle<Function>::Cast(value);

        while (1)
        {
            ReadMessage (isolate, 3, &message);
            args[0] = message;

            func->Call(isolate->GetCurrentContext()->Global(), 1, args);
        }
    }
}


/** Send the message out of the sandbox.
      This function is callable in the contract as 'postMessage'.
      This represents an asynchronous call from the contract.
*/
static void PostMessageCallback (FunctionCallbackInfo<Value> const& args)
{
    if (args.Length() == 1 && args[0]->IsString())
    {
        String::Utf8Value message(args[0]);
        WriteMessage (3, *message);
    }
}


/** Run the JavaScript contract code using V8. */
void run_js_contract(char const* contract)
{
    Isolate* isolate = Isolate::New();
    Isolate::Scope isolate_scope(isolate);

    // Create a stack-allocated handle scope.
    HandleScope handle_scope(isolate);

    // Add global functions defined in C. 
    Handle<ObjectTemplate> global = ObjectTemplate::New();
    global->Set (String::NewFromUtf8(isolate, "postMessage"),
                 FunctionTemplate::New(isolate, PostMessageCallback));

    // Create a new context.
    Handle<Context> context = Context::New(isolate, NULL, global);

    // Enter the context for compiling and running the contract script.
    Context::Scope context_scope(context);

    // Create a string containing the JavaScript source code.
    Handle<String> source = String::NewFromUtf8(isolate, contract);

    // Compile the source code.
    Handle<Script> script = Script::Compile(source);
        
    // Run the script.
    script->Run();

    ReadMessages (isolate);  
}


/** Pass in the JavaScript contract as an argument. */
int main(int argc, char* argv[])
{
    if (argc>=2)
    {
        run_js_contract(argv[1]);
    }

    return 0;
};