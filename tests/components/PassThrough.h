#pragma once

namespace DSPatch
{

class PassThrough : public Component
{
public:
    PassThrough()
        : Component( ProcessOrder::OutOfOrder )
    {
        SetInputCount_( 1 );
        SetOutputCount_( 1 );
    }

protected:
    virtual void Process_( SignalBus& inputs, SignalBus& outputs ) override
    {
        const auto* in = inputs.GetValue<int>( 0 );
        if ( in )
        {
            outputs.MoveSignal( 0, *inputs.GetSignal( 0 ) );  // pass the signal through (no copy)
        }
        // else set no output
    }
};

}  // namespace DSPatch
