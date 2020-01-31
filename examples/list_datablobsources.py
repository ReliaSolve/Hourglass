# Import the hrgls library
import hrgls,sys

# Open an API object with default parameters and verify that its status is okay.
api = hrgls.API()
s = api.GetStatus()
if (s != hrgls.hrgls_STATUS_OKAY):
    print('Could not open API with default parameters:')
    print(hrgls.hrgls_ErrorMessage(s))
    sys.exit(1)

dbs = api.GetAvailableDataBlobSources()
for db in dbs:
    print('Renderer ', db.Name())
