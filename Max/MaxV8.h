/*
 // Copyright (c) 2015 Eliott Paris, CICM, Universite Paris 8.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#ifndef _MAX_V8_H_
#define _MAX_V8_H_

/* @todo :
 - clean all that code
 - make inletassist/outletassist work
 - implement messagename
 - support arrayfromargs(messagename, arguments)
 - call anything or list if defined
 - ...
 */

extern "C"
{
#include "ext.h"
#include "ext_obex.h"
}

#include <map>
#include <vector>
#include <string>
#include <iostream>

#include "include/v8.h"
#include "include/libplatform/libplatform.h"

namespace cicm
{
    using namespace v8;
    using namespace std;
    
    class MaxV8
    {
    public:
        //! Init v8.
        static void Init();
        
        //! Release v8.
        static void Release();
        
        //! Allocates a new instance.
        static void* NewInstance(t_symbol* s, long argc, t_atom* argv);
        
        //! Free an instance.
        static void FreeInstance(MaxV8* x);
        
        //! Max assist method wrapper.
        static void Assist(MaxV8* x, void* b, long m, long a, char* s);
        
        //! read a file, then if succeed compile and run script
        static void Read(MaxV8* x, t_symbol *s);
        
        //! loadbang method
        static void Loadbang(MaxV8* x);
        
        //! bang method
        static void Bang(MaxV8* x);
        
        //! int method
        static void Int(MaxV8* x, long number);
        
        //! float method
        static void Float(MaxV8* x, double number);
        
        //! bang method
        static void Anything(MaxV8* x, t_symbol *s, long ac, t_atom *av);
        
        //! method to open the text editor
        static void OpenEditor(MaxV8* x);
        
        //! callback method called by Max when editor has been closed
        static void EditorClosed(MaxV8* x, char **text, long size);
        
        //! callback method called by Max when editor has been saved
        static long EditorSaved(MaxV8* x, char **text, long size);
        
        static t_class* obj_class;
        t_object obj;
        
    private:
        
        long                m_obj_argc;
        t_atom*             m_obj_argv;
        long                m_inletcount;
        char                m_filename[MAX_PATH_CHARS];
        short               m_path;
        t_handle            m_text;
        long                m_textsize;
        t_object*           m_texteditor;
        vector<void*>       m_outlets;
        
        int                 m_number_of_inlets;
        int                 m_number_of_outlets;
        map<int, string>    m_inlet_assist;
        map<int, string>    m_outlet_assist;
        
        bool                m_script_compiled;
        static v8::Platform *v8_platform;
        v8::Isolate*        m_isolate;
        v8::Persistent
        <v8::Context>       m_js_context;
        
        //---------------------------------------------
                
        static void DoRead(MaxV8* x, t_symbol *s, long argc, t_atom *argv);
        
        // Creates a new execution environment containing the Max wrapped functions.
        Local<Context> createMaxContext(Isolate* isolate);
        
        //! Compile and run the given script
        Local<Value> compileAndRunScript(Isolate* isolate, Local<v8::String> script);
        
        //! Compile and run the current script
        static void CompileAndRun(MaxV8 *x);
        
        //! resize the inlets and outlets
        static void ResizeIO(MaxV8 *x, long last_ins, long new_ins, long last_outs, long new_outs);
        
        //------------------------------------------------------------------------
        // v8 static handles
        //------------------------------------------------------------------------
        
        //! call a named JavaScript function with arguments
        static Local<Value> CallJsFunction(MaxV8* x, t_symbol *s, long ac, t_atom *av);
        
        static void JsInletsGetter(Local<String> property, const PropertyCallbackInfo<Value>& info);
        static void JsInletsSetter(Local<String> property, Local<Value> value, const PropertyCallbackInfo<void>& info);
        
        static void JsOutletsGetter(Local<String> property, const PropertyCallbackInfo<Value>& info);
        static void JsOutletsSetter(Local<String> property, Local<Value> value, const PropertyCallbackInfo<void>& info);
        
        static void JsArgumentsGetter(Local<String> property, const PropertyCallbackInfo<Value>& info);
        
        static void JsProxyInletsGetter(Local<String> property, const PropertyCallbackInfo<Value>& info);
        
        static void JsSetInletAssist(const FunctionCallbackInfo<Value>& args);
        static void JsSetOutletAssist(const FunctionCallbackInfo<Value>& args);
        
        //! JavaScript 'outlet' function wrapper.
        static void JsOutput(FunctionCallbackInfo<Value> const& args);
        
        //! JavaScript 'outlet' function wrapper.
        static void JsArrayFromArgs(FunctionCallbackInfo<Value> const& args);
        
        static vector<t_atom> ValueToAtomVector(Local<Context> context, Local<Value> value);
        
        //! @internal Extracts a C string from a V8 Utf8Value.
        static const char* ToCString(String::Utf8Value const& value);
        
        //! JavaScript 'post' function wrapper.
        static void JsPost(FunctionCallbackInfo<Value> const& args);
        
        //! JavaScript 'error' function wrapper.
        static void JsError(FunctionCallbackInfo<Value> const& args);
    };
}

#endif // _MAX_V8_H_
