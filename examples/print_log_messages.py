# Import the hrgls library
import hrgls
import sys
import ctypes
import time

# Handler for callback-based reading of log messages
def HandleLogCallback(message, timeToQuit) :
    #Do whatever we want with the message.
    ## @todo Replace this code with whatever is desired.
    HandleLogCallback.count += 1
    print('Callback message with level',message.Level(),'received:',message.Value())

# Behaves like a static variable and retains its value between calls.
HandleLogCallback.count = 0

# Open an API object with default parameters and verify that its status is okay.
api = hrgls.API()
status = api.GetStatus()
if (status != hrgls.hrgls_STATUS_OKAY):
    print('Could not open API: ', hrgls.hrgls_ErrorMessage(status))
    sys.exit(1)

# This shows how to read the messages by callback handler.

# We must pass a list (or other mutable object) as the user-data
# parameter to the callback handler so that when it changes the
# value we can see that it has changed.  Here, we pass a single-
# element list whose 0th element represents whether it is time
# to quit.  We use a global object to make sure that it does not
# go out of scope before the callback handler is released.
done = [False]
api.SetLogMessageCallback(HandleLogCallback, done)
status = api.GetStatus()
if (status != hrgls.hrgls_STATUS_OKAY):
    print('Could not set callback handler: ', hrgls.hrgls_ErrorMessage(status))
    sys.exit(2)

# Start the messages streaming and then read and parse them.
api.SetLogMessageStreamingState(True)
status = api.GetStatus()
if (status != hrgls.hrgls_STATUS_OKAY):
    print('Could not set streaming state on: ', hrgls.hrgls_ErrorMessage(status))
    sys.exit(3)

# Run until we have 5 messages or we time out.
start = api.GetCurrentSystemTime()
while HandleLogCallback.count < 5:
    now = api.GetCurrentSystemTime();
    if (now.tv_sec - start.tv_sec > 5):
        print('Timeout waiting for callback-based messages.')
        sys.exit(4)

# Unhook the callback handler after stopping the stream.
api.SetLogMessageStreamingState(False);
status = api.GetStatus()
if (status != hrgls.hrgls_STATUS_OKAY):
    print('Could not set streaming state off: ', hrgls.hrgls_ErrorMessage(status))
    sys.exit(5)
api.SetLogMessageCallback(None, None);
status = api.GetStatus()
if (status != hrgls.hrgls_STATUS_OKAY):
    print('Could not reset callback handler: ', hrgls.hrgls_ErrorMessage(status))
    sys.exit(6)

# This shows how to read messages directly.

# Set a minimum threshold for the messages at warning for the
# get-based test and ensure that we don't get any messages with
# lower level than that.
api.SetLogMessageMinimumLevel(hrgls.hrgls_MESSAGE_MINIMUM_WARNING)
status = api.GetStatus()
if (status != hrgls.hrgls_STATUS_OKAY):
    print('Could not set minimum message level: ', hrgls.hrgls_ErrorMessage(status))
    sys.exit(100)

# Start the messages streaming and then read and parse them.
api.SetLogMessageStreamingState(True)
status = api.GetStatus()
if (status != hrgls.hrgls_STATUS_OKAY):
    print('Could not set streaming state on: ', hrgls.hrgls_ErrorMessage(status))
    sys.exit(7)
count = 0;
start = api.GetCurrentSystemTime()
while (count < 5):
    messages = api.GetPendingLogMessages()
    status = api.GetStatus()
    if (status == hrgls.hrgls_STATUS_OKAY):
        count = count + len(messages)

        # Make sure we don't get any messages below the minimum level
        for message in messages:
            if message.Level() < hrgls.hrgls_MESSAGE_MINIMUM_WARNING:
                print('Message received with too-low level:',message.Level())
                sys.exit(101)
            print('Get-based message with level',message.Level(),'received:',message.Value())
    else:
        if (status != hrgls.hrgls_STATUS_TIMEOUT):
            print('Error reading messages: ', hrgls.hrgls_ErrorMessage(status))
            sys.exit(8)

    end = api.GetCurrentSystemTime()
    if (end.tv_sec - start.tv_sec > 5):
        print('Timeout waiting for get-based messages.')
        sys.exit(9)

# Stop streaming
api.SetLogMessageStreamingState(True)
status = api.GetStatus()
if (status != hrgls.hrgls_STATUS_OKAY):
    print('Could not set streaming state on: ', hrgls.hrgls_ErrorMessage(status))
    sys.exit(10)

print('Success!')

