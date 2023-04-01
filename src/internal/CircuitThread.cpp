/******************************************************************************
DSPatch - The Refreshingly Simple C++ Dataflow Framework
Copyright (c) 2023, Marcus Tomlinson

BSD 2-Clause License

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/

#include <internal/CircuitThread.h>

using namespace DSPatch::internal;

CircuitThread::CircuitThread()
{
}

// cppcheck-suppress missingMemberCopy
CircuitThread::CircuitThread( CircuitThread&& )
{
}

CircuitThread::~CircuitThread()
{
    Stop();
}

void CircuitThread::Start( std::vector<DSPatch::Component::SPtr>* components, int bufferNo, int threadsPerBuffer )
{
    for ( const auto& thread : _threads )
    {
        if ( !thread.stopped )
        // cppcheck-suppress useStlAlgorithm
        {
            return;
        }
    }

    _components = components;
    _bufferNo = bufferNo;

    _threads.resize( threadsPerBuffer );
    for ( size_t i = 0; i < _threads.size(); ++i )
    {
        _threads[i].stop = false;
        _threads[i].stopped = false;
        _threads[i].gotResume = false;
        _threads[i].gotSync = false;

        _threads[i].thread = std::thread( &CircuitThread::_Run, this, &_threads[i] );
    }
}

void CircuitThread::Stop()
{
    for ( const auto& thread : _threads )
    {
        if ( thread.stopped )
        // cppcheck-suppress useStlAlgorithm
        {
            return;
        }
    }

    Sync();

    for ( auto& thread : _threads )
    {
        thread.stop = true;
    }

    SyncAndResume( _mode );

    for ( auto& thread : _threads )
    {
        if ( thread.thread.joinable() )
        {
            thread.thread.join();
        }
    }
}

void CircuitThread::Sync()
{
    for ( auto& thread : _threads )
    {
        if ( thread.stopped )
        {
            return;
        }

        if ( thread.gotSync )
        {
            continue;
        }

        std::unique_lock<std::mutex> lock( thread.resumeMutex );

        if ( !thread.gotSync )              // if haven't already got sync
        {
            thread.syncCondt.wait( lock );  // wait for sync
        }
    }
}

void CircuitThread::SyncAndResume( DSPatch::Component::TickMode mode )
{
    for ( auto& thread : _threads )
    {
        if ( thread.stopped )
        {
            return;
        }

        if ( thread.gotSync )
        {
            std::lock_guard<std::mutex> lock( thread.resumeMutex );
            thread.gotSync = false;  // reset the sync flag
            continue;
        }

        std::unique_lock<std::mutex> lock( thread.resumeMutex );

        if ( !thread.gotSync )              // if haven't already got sync
        {
            thread.syncCondt.wait( lock );  // wait for sync
        }
        thread.gotSync = false;             // reset the sync flag
    }

    _mode = mode;

    for ( auto& thread : _threads )
    {
        thread.gotResume = true;  // set the resume flag
        thread.resumeCondt.notify_all();
    }
}

void CircuitThread::_Run( Thread* thread )
{
    if ( _components != nullptr )
    {
        {
            std::unique_lock<std::mutex> lock( thread->resumeMutex );

            thread->gotSync = true;  // set the sync flag
            thread->syncCondt.notify_all();

            if ( !thread->gotResume )              // if haven't already got resume
            {
                thread->resumeCondt.wait( lock );  // wait for resume
            }
            thread->gotResume = false;             // reset the resume flag
        }

        while ( !thread->stop )
        {
            // You might be thinking: Can't we have each thread start on a different component?

            // Well no. Because threadNo == bufferNo, in order to maintain synchronisation
            // within the circuit, when a component wants to process its buffers in-order, it
            // requires that every other in-order component in the system has not only
            // processed its buffers in the same order, but has processed the same number of
            // buffers too.

            // E.g. 1,2,3 and 1,2,3. Not 1,2,3 and 2,3,1,2,3.

            for ( auto& component : *_components )
            {
                component->Tick( _mode, _bufferNo );
            }

            {
                std::unique_lock<std::mutex> lock( thread->resumeMutex );

                thread->gotSync = true;  // set the sync flag
                thread->syncCondt.notify_all();

                if ( !thread->gotResume )              // if haven't already got resume
                {
                    thread->resumeCondt.wait( lock );  // wait for resume
                }
                thread->gotResume = false;             // reset the resume flag
            }

            for ( auto& component : *_components )
            {
                component->Reset( _bufferNo );
            }
        }
    }

    thread->stopped = true;
}
