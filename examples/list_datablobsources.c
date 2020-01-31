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
  status = hrgls_APICreateParametersDestroy(params);
  if (status != hrgls_STATUS_OKAY) {
    fprintf(stderr, "Could not destroy API open parameters: %s\n",
      hrgls_ErrorMessage(status));
    return 1000;
  }

  // Find out how many DataBlobSources are in the system, which will also latch the data
  // on the DataBlobSources.
  uint32_t count;
  if (hrgls_STATUS_OKAY != (status = hrgls_APIGetAvailableDataBlobSourceCount(api, &count))) {
    fprintf(stderr, "Could not get number of available DataBlobSources: %s\n",
      hrgls_ErrorMessage(status));
    return 3;
  }
  printf("Found %d DataBlobSources\n", count);

  // Read each DataBlobSource's information.
  for (uint32_t i = 0; i < count; i++) {
    hrgls_APIDataBlobSourceInfo dbsInfo;
    if (hrgls_STATUS_OKAY != (status = hrgls_APIGetAvailableDataBlobSourceInfo(api, i, &dbsInfo))) {
      fprintf(stderr, "Could not get DataBlobSource info for DataBlobSource %d: %s\n", i,
        hrgls_ErrorMessage(status));
      return 301;
    }

    // Get and print the DataBlobSource name.
    const char *name;
    if (hrgls_STATUS_OKAY != (status = hrgls_APIDataBlobSourceGetName(dbsInfo, &name))) {
      fprintf(stderr, "Could not get DataBlobSource name for DataBlobSource %d: %s\n", i,
        hrgls_ErrorMessage(status));
      return 302;
    }
    printf(" DataBlobSource name: %s\n", name);
  }

  status = hrgls_APIDestroy(api);
  if (status != hrgls_STATUS_OKAY) {
    fprintf(stderr, "Could not destroy API with default parameters: %s\n",
      hrgls_ErrorMessage(status));
    return 20;
  }

  return 0;
}

