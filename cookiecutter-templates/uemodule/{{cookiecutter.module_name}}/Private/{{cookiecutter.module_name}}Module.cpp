// {{cookiecutter.copyright}}

#include "{{cookiecutter.module_name}}Module.h"

#define LOCTEXT_NAMESPACE "F{{cookiecutter.module_name}}Module"

F{{cookiecutter.module_name}}Module::F{{cookiecutter.module_name}}Module()
{
}


F{{cookiecutter.module_name}}Module::~F{{cookiecutter.module_name}}Module()
{
}


void F{{cookiecutter.module_name}}Module::StartupModule()
{
}

void F{{cookiecutter.module_name}}Module::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(F{{cookiecutter.module_name}}Module, {{cookiecutter.module_name}})
