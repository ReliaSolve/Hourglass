/*
 * Copyright 2020 ReliaSolve, Inc.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
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

