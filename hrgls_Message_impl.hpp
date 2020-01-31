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
* @file hrgls_Message_impl.hpp
* @brief Internal implementation file.
*
* This is an internal wrapper file that should not be directly included
* by application code or by code that implements the API.
* @author Russell Taylor.
* @date January 18, 2020.
*/

namespace hrgls {

  class Message::Message_private {
  public:
    hrgls_Message Message = nullptr;
    hrgls_Status status = hrgls_STATUS_OKAY;
  };

  Message::Message()
  {
    m_private = new Message_private;
    hrgls_Message obj = nullptr;
    m_private->status = hrgls_MessageCreate(&obj);
    m_private->Message = obj;
  }

  Message::Message(::std::string value, struct timeval time,
	  hrgls_MessageLevel level)
  {
    m_private = new Message_private;
    hrgls_Message obj;
    m_private->status = hrgls_MessageCreate(&obj);
    m_private->Message = obj;
  	hrgls_Status s = hrgls_STATUS_OKAY;
	  s = hrgls_MessageSetValue(m_private->Message, value.c_str());
	  if (s != hrgls_STATUS_OKAY) {
		  m_private->status = s;
	  }
	  s = hrgls_MessageSetTimeStamp(m_private->Message, time);
	  if (s != hrgls_STATUS_OKAY) {
		  m_private->status = s;
	  }
	  s = hrgls_MessageSetLevel(m_private->Message, level);
	  if (s != hrgls_STATUS_OKAY) {
		  m_private->status = s;
	  }
  }

  Message::Message(hrgls_Message Message)
  {
    m_private = new Message_private;
    if (!Message) {
      m_private->status = hrgls_STATUS_NULL_OBJECT_POINTER;
      return;
    }
    m_private->status = hrgls_MessageCopy(&m_private->Message, Message);
  }

  Message::~Message()
  {
    if (m_private) {
      if (m_private->Message) {
        hrgls_MessageDestroy(m_private->Message);
      }
      delete m_private;
    }
  }

  Message::Message(const Message &copy)
  {
    m_private = new Message_private();
    m_private->status = hrgls_MessageCopy(&m_private->Message, copy.RawMessage());
  }

  Message &Message::operator = (const Message &copy)
  {
    if (m_private) {
      if (m_private->Message) {
        hrgls_MessageDestroy(m_private->Message);
      }
      delete m_private;
    }
    m_private = new Message_private();
    m_private->status = hrgls_MessageCopy(&m_private->Message, copy.RawMessage());
    return *this;
  }

  hrgls_Status Message::GetStatus()
  {
    if (!m_private) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    return m_private->status;
  }

  ::std::string Message::Value() const
  {
    ::std::string ret;
    if (!m_private->Message) {
      m_private->status = hrgls_STATUS_NULL_OBJECT_POINTER;
      return ret;
    }
    const char *val = "";
    m_private->status = hrgls_MessageGetValue(m_private->Message, &val);
    ret = val;
    return ret;
  }

  hrgls_Status Message::Value(::std::string val)
  {
    if (!m_private || !m_private->Message) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    return hrgls_MessageSetValue(m_private->Message, val.c_str());
  }

  struct timeval Message::TimeStamp() const
  {
	  struct timeval ret = {};
	  if (!m_private->Message) {
		  m_private->status = hrgls_STATUS_NULL_OBJECT_POINTER;
		  return ret;
	  }
	  m_private->status = hrgls_MessageGetTimeStamp(m_private->Message, &ret);
	  return ret;
  }

  hrgls_Status Message::TimeStamp(struct timeval val)
  {
    if (!m_private || !m_private->Message) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    return hrgls_MessageSetTimeStamp(m_private->Message, val);
  }

  hrgls_MessageLevel Message::Level() const
  {
	  hrgls_MessageLevel ret = {};
	  if (!m_private->Message) {
		  m_private->status = hrgls_STATUS_NULL_OBJECT_POINTER;
		  return ret;
	  }
	  m_private->status = hrgls_MessageGetLevel(m_private->Message, &ret);
	  return ret;
  }

  hrgls_Status Message::Level(hrgls_MessageLevel val)
  {
    if (!m_private || !m_private->Message) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    return hrgls_MessageSetLevel(m_private->Message, val);
  }

  hrgls_Message const Message::RawMessage() const
  {
    hrgls_Message ret = nullptr;
    if (!m_private) {
      return ret;
    }
    if (!m_private->Message) {
      m_private->status = hrgls_STATUS_NULL_OBJECT_POINTER;
      return ret;
    }
    m_private->status = hrgls_STATUS_OKAY;
    return m_private->Message;
  }

} // End namespace hrgls
