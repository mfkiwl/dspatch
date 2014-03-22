/************************************************************************
DSPatch - Cross-Platform, Object-Oriented, Flow-Based Programming Library
Copyright (c) 2012-2013 Marcus Tomlinson

This file is part of DSPatch.

GNU Lesser General Public License Usage
This file may be used under the terms of the GNU Lesser General Public
License version 3.0 as published by the Free Software Foundation and
appearing in the file LGPLv3.txt included in the packaging of this
file. Please review the following information to ensure the GNU Lesser
General Public License version 3.0 requirements will be met:
http://www.gnu.org/copyleft/lgpl.html.

Other Usage
Alternatively, this file may be used in accordance with the terms and
conditions contained in a signed written agreement between you and
Marcus Tomlinson.

DSPatch is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
************************************************************************/

#include <dspatch/DspPlugin.h>

//=================================================================================================

DspPluginLoader::DspPluginLoader( std::string const& pluginPath )
  : _handle( NULL )
{
  // open library
  _handle = dlopen( pluginPath.c_str(), RTLD_NOW );
  if( _handle )
  {
    // load symbols
    _getCreateParams = ( GetCreateParams_t ) dlsym( _handle, "GetCreateParams" );
    _create = ( Create_t ) dlsym( _handle, "Create" );

    if( !_getCreateParams || !_create )
    {
      dlclose( _handle );
      _handle = NULL;
    }
  }
}

DspPluginLoader::~DspPluginLoader()
{
  // close library
  if( _handle )
  {
    dlclose( _handle );
  }
}

//=================================================================================================

bool DspPluginLoader::IsLoaded()
{
  return _handle;
}

//-------------------------------------------------------------------------------------------------

std::map< std::string, DspParameter > DspPluginLoader::GetCreateParams()
{
  if( _handle )
  {
    return _getCreateParams();
  }
  return std::map< std::string, DspParameter >();
}

//-------------------------------------------------------------------------------------------------

DspComponent* DspPluginLoader::Create( std::map< std::string, DspParameter > const& params )
{
  if( _handle )
  {
    return _create( params );
  }
  return NULL;
}

//=================================================================================================
