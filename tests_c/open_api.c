/*
 * Copyright 2020 ReliaSolve, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <hrgls_api.h>

int main(int argc, const char *argv[])
{
  // Get a base API object, specifying default parameters.
  hrgls_Status status;
  hrgls_APICreateParams params;
  status = hrgls_APICreateParametersCreate(&params);
  if (status != hrgls_STATUS_OKAY) {
    fprintf(stderr, "Could not create API open parameters: %s\n",
      hrgls_ErrorMessage(status));
    return 1;
  }

  hrgls_API api;
  status = hrgls_APICreate(&api, params);
  if (status != hrgls_STATUS_OKAY) {
    fprintf(stderr, "Could not create API with default parameters: %s\n",
      hrgls_ErrorMessage(status));
    return 2;
  }
  status = hrgls_APIDestroy(api);
  if (status != hrgls_STATUS_OKAY) {
    fprintf(stderr, "Could not destroy API with default parameters: %s\n",
      hrgls_ErrorMessage(status));
    return 20;
  }

  // Get a base API object, specifying a name, and non-empty
  // credentials.
  status = hrgls_APICreateParametersSetName(params, "Test");
  if (status != hrgls_STATUS_OKAY) {
    fprintf(stderr, "Could not set name in API open parameters: %s\n",
      hrgls_ErrorMessage(status));
    return 3;
  }
  uint8_t myCredentials[] = "Credentials";
  status = hrgls_APICreateParametersSetCredentials(params, myCredentials, sizeof(myCredentials));
  if (status != hrgls_STATUS_OKAY) {
    fprintf(stderr, "Could not set credentials in API open parameters: %s\n",
      hrgls_ErrorMessage(status));
    return 4;
  }
  hrgls_API api2;
  status = hrgls_APICreate(&api2, params);
  if (status != hrgls_STATUS_OKAY) {
    fprintf(stderr, "Could not Open API with name and credentials: %s\n",
      hrgls_ErrorMessage(status));
    return 7;
  }
  hrgls_VERSION version;
  status = hrgls_APIGetVersion(api2, &version);
  if (status != hrgls_STATUS_OKAY) {
    fprintf(stderr, "Could not get version: %s\n",
      hrgls_ErrorMessage(status));
    return 8;
  }
  status = hrgls_APISetVerbosity(api2, 201);
  if (status != hrgls_STATUS_OKAY) {
    fprintf(stderr, "Could not set verbosity: %s\n",
      hrgls_ErrorMessage(status));
    return 9;
  }
  status = hrgls_APIDestroy(api2);
  if (status != hrgls_STATUS_OKAY) {
    fprintf(stderr, "Could not destroy API with name and credentials: %s\n",
      hrgls_ErrorMessage(status));
    return 70;
  }

  status = hrgls_APICreateParametersDestroy(params);
  if (status != hrgls_STATUS_OKAY) {
    fprintf(stderr, "Could not destroy API parameters: %s\n",
      hrgls_ErrorMessage(status));
    return 8;
  }

  printf("Success!\n");
  return 0;
}
