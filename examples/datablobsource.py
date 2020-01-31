# Import the hrgls library
import hrgls
import sys
import ctypes
import time

# Handler for callback-based reading of DataBlob
def HandleBlobCallback(blob, timeToQuit) :

    #Do whatever we want with the blob.
    ## @todo Replace this code with whatever is desired.
    HandleBlobCallback.count += 1
    if (HandleBlobCallback.count >= 10):
        timeToQuit[0] = True
    blob.ReleaseData()
    status = blob.GetStatus()
    if (status != hrgls.hrgls_STATUS_OKAY):
        print('Could not release blob data in callback: ', hrgls.hrgls_ErrorMessage(status))
        timeToQuit[0] = True

# Behaves like a static variable and retains its value between calls.
HandleBlobCallback.count = 0

# Open an API object with default parameters and verify that its status is okay.
api = hrgls.API()
status = api.GetStatus()
if (status != hrgls.hrgls_STATUS_OKAY):
    print('Could not open API: ', hrgls.hrgls_ErrorMessage(status))
    sys.exit(1)

# Create a stream using default stream properties.
sp = hrgls.StreamProperties()
status = sp.GetStatus()
if (status != hrgls.hrgls_STATUS_OKAY):
    print('Could not create stream properties: ', hrgls.hrgls_ErrorMessage(status))
    sys.exit(7)
stream = hrgls.DataBlobSource(api, sp)
status = stream.GetStatus()
if (status != hrgls.hrgls_STATUS_OKAY):
    print('Could not create stream: ', hrgls.hrgls_ErrorMessage(status))
    sys.exit(9)

# This shows how to read the blobs by grabbing them one at a time.
print('Callback-based blob reading')

# We must pass a list (or other mutable object) as the user-data
# parameter to the callback handler so that when it changes the
# value we can see that it has changed.  Here, we pass a single-
# element list whose 0th element represents whether it is time
# to quit.  We make this object global so that it won't go out
# of scope before the callback handler is released.
done = [False]
if (stream.SetStreamCallback(HandleBlobCallback, done) != hrgls.hrgls_STATUS_OKAY):
    print('Could not set callback handler: ', hrgls.hrgls_ErrorMessage(status))
    sys.exit(11)

# Start streaming and then read and parse the blobs.
if (stream.SetStreamingState(True) != hrgls.hrgls_STATUS_OKAY):
    print('Could not set streaming state on: ', hrgls.hrgls_ErrorMessage(status))
    sys.exit(12)

# Run until the callback handler sets done.
while (not done[0]):
    time.sleep(0.1)

# Unhook the callback handler after stopping the stream.
if (stream.SetStreamingState(False) != hrgls.hrgls_STATUS_OKAY):
    print('Could not set streaming state off: ', hrgls.hrgls_ErrorMessage(status))
    sys.exit(13)
if (stream.SetStreamCallback(None, None) != hrgls.hrgls_STATUS_OKAY):
    print('Could not reset callback handler: ', hrgls.hrgls_ErrorMessage(status))
    sys.exit(14)

# This shows how to read the blobs using a callback handler.
print('Get-based blob reading')

# Start streaming and then read and parse the blobs.
if (stream.SetStreamingState(True) != hrgls.hrgls_STATUS_OKAY):
    print('Could not set streaming state on: ', hrgls.hrgls_ErrorMessage(status))
    sys.exit(15)
count = 0
while (count <= 10):
    blob = stream.GetNextBlob()
    status = stream.GetStatus()
    if (status == hrgls.hrgls_STATUS_OKAY):
        count = count + 1

        # Convert the data we received into a bytearray so that
        # we can deal with it in a manner useful to Python
        myCArray = ctypes.cast(int(blob.Data()), ctypes.POINTER(ctypes.c_char * blob.Size()))[0]
        myArray  = bytearray(myCArray)
        if (len(myArray) >= 2):
            print(' first char = ',myArray[0])
            print(' second char = ',myArray[1])

        # Release the data from the blob.
        blob.ReleaseData()
        status = blob.GetStatus()
        if (status != hrgls.hrgls_STATUS_OKAY):
            print('Could not release blob data main program: ', hrgls.hrgls_ErrorMessage(status))
            sys.exit(17)
    else:
        if (status != hrgls.hrgls_STATUS_TIMEOUT):
            print('Bad blob received: ', hrgls.hrgls_ErrorMessage(status))
            sys.exit(18)

# Stop streaming
if (stream.SetStreamingState(False) != hrgls.hrgls_STATUS_OKAY):
    print('Could not set streaming state off: ', hrgls.hrgls_ErrorMessage(status))
    sys.exit(19)

print('Success!')
