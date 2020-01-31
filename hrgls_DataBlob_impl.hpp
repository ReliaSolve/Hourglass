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

#pragma once

/**
* @file hrgls_DataBlob_impl.hpp
* @brief Internal implementation file.
*
* This is an internal wrapper file that should not be directly included
* by application code or by code that implements the API.
* @author Russell Taylor.
* @date January 18, 2020.
*/

#include <string.h>

namespace hrgls {

  namespace datablob {

    class DataBlob::DataBlob_private{
    public:
      hrgls_DataBlob blob = nullptr;
      hrgls_Status status = hrgls_STATUS_OKAY;
    };

    DataBlob::DataBlob()
    {
      m_private = new DataBlob_private;
      m_private->status = hrgls_DataBlobCreate(&m_private->blob);
    }

    DataBlob::DataBlob(hrgls_DataBlob blob)
    {
      m_private = new DataBlob_private;
      if (!blob) {
        m_private->status = hrgls_STATUS_BAD_PARAMETER;
        return;
      }
      m_private->status = hrgls_DataBlobCopy(&m_private->blob, blob);
    }

    DataBlob::~DataBlob()
    {
      if (m_private) {
        if (m_private->blob) {
          hrgls_DataBlobDestroy(m_private->blob);
        }
        delete m_private;
      }
    }

    DataBlob::DataBlob(const DataBlob&copy)
    {
      m_private = new DataBlob_private();
      m_private->status = hrgls_DataBlobCopy(&m_private->blob, copy.RawDataBlob());
    }

    DataBlob& DataBlob::operator = (const DataBlob&copy)
    {
      // Get rid of any pre-existing data that we have.
      if (m_private) {
        if (m_private->blob) {
          hrgls_DataBlobDestroy(m_private->blob);
        }
        delete m_private;
      }

      m_private = new DataBlob_private();
      m_private->status = hrgls_DataBlobCopy(&m_private->blob, copy.RawDataBlob());
      return *this;
    }

    hrgls_Status DataBlob::GetStatus()
    {
      if (!m_private) {
        return hrgls_STATUS_NULL_OBJECT_POINTER;
      }
      return m_private->status;
    }

    struct timeval DataBlob::Time() const
    {
      struct timeval ret = {};
      if (!m_private->blob) {
        m_private->status = hrgls_STATUS_NULL_OBJECT_POINTER;
        return ret;
      }
      if (hrgls_STATUS_OKAY !=
        (m_private->status = hrgls_DataBlobGetTime(m_private->blob, &ret))) {
        return ret;
      }
      return ret;
    }

    const uint8_t * DataBlob::Data() const
    {
      const uint8_t *ret = nullptr;
      if (!m_private->blob) {
        m_private->status = hrgls_STATUS_NULL_OBJECT_POINTER;
        return ret;
      }
      uint32_t size;
      m_private->status = hrgls_DataBlobGetData(m_private->blob, &ret, &size);
      return ret;
    }

    uint32_t DataBlob::Size() const
    {
      uint32_t ret = 0;
      if (!m_private->blob) {
        m_private->status = hrgls_STATUS_NULL_OBJECT_POINTER;
        return ret;
      }
      const uint8_t *data;
      m_private->status = hrgls_DataBlobGetData(m_private->blob, &data, &ret);
      return ret;
    }

    void DataBlob::ReleaseData()
    {
      if (!m_private->blob) {
        m_private->status = hrgls_STATUS_NULL_OBJECT_POINTER;
        return;
      }
      m_private->status = hrgls_DataBlobReleaseData(m_private->blob);
      return;
    }

    hrgls_DataBlob const DataBlob::RawDataBlob() const
    {
      hrgls_DataBlob ret = nullptr;
      if (!m_private) {
        return ret;
      }
      if (!m_private->blob) {
        m_private->status = hrgls_STATUS_NULL_OBJECT_POINTER;
        return ret;
      }
      m_private->status = hrgls_STATUS_OKAY;
      return m_private->blob;
    }

  } // End namespace datablob

} // End namespace hrgls

