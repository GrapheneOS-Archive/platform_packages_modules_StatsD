/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "FieldValue.h"

#include <android/frameworks/stats/1.0/types.h>
#include <android/util/ProtoOutputStream.h>
#include <log/log_read.h>
#include <private/android_logger.h>
#include <stats_event_list.h>
#include <stats_event.h>
#include <utils/Errors.h>

#include <string>
#include <vector>

using namespace android::frameworks::stats::V1_0;

namespace android {
namespace os {
namespace statsd {

struct AttributionNodeInternal {
    void set_uid(int32_t id) {
        mUid = id;
    }

    void set_tag(const std::string& value) {
        mTag = value;
    }

    int32_t uid() const {
        return mUid;
    }

    const std::string& tag() const {
        return mTag;
    }

    int32_t mUid;
    std::string mTag;
};

struct InstallTrainInfo {
    int64_t trainVersionCode;
    std::string trainName;
    int32_t status;
    std::vector<int64_t> experimentIds;
};

/**
 * Wrapper for the log_msg structure.
 */
class LogEvent {
public:
    /**
     * Read a LogEvent from the socket
     */
    explicit LogEvent(uint8_t* msg, uint32_t len, int32_t uid, int32_t pid);

    /**
     * Temp constructor to use for pulled atoms until we flip the socket schema.
     */
    explicit LogEvent(uint8_t* msg, uint32_t len, int32_t uid, int32_t pid, bool useNewSchema);

    /**
     * Constructs a LogEvent with synthetic data for testing. Must call init() before reading.
     */
    explicit LogEvent(int32_t tagId, int64_t wallClockTimestampNs, int64_t elapsedTimestampNs);

    // For testing. The timestamp is used as both elapsed real time and logd timestamp.
    explicit LogEvent(int32_t tagId, int64_t timestampNs);

    // For testing. The timestamp is used as both elapsed real time and logd timestamp.
    explicit LogEvent(int32_t tagId, int64_t timestampNs, int32_t uid);

    /**
     * Constructs a KeyValuePairsAtom LogEvent from value maps.
     */
    explicit LogEvent(int32_t tagId, int64_t wallClockTimestampNs, int64_t elapsedTimestampNs,
                      int32_t uid,
                      const std::map<int32_t, int32_t>& int_map,
                      const std::map<int32_t, int64_t>& long_map,
                      const std::map<int32_t, std::string>& string_map,
                      const std::map<int32_t, float>& float_map);

    // Constructs a BinaryPushStateChanged LogEvent from API call.
    explicit LogEvent(const std::string& trainName, int64_t trainVersionCode, bool requiresStaging,
                      bool rollbackEnabled, bool requiresLowLatencyMonitor, int32_t state,
                      const std::vector<uint8_t>& experimentIds, int32_t userId);

    explicit LogEvent(int64_t wallClockTimestampNs, int64_t elapsedTimestampNs,
                      const VendorAtom& vendorAtom);

    explicit LogEvent(int64_t wallClockTimestampNs, int64_t elapsedTimestampNs,
                      const InstallTrainInfo& installTrainInfo);

    ~LogEvent();

    /**
     * Get the timestamp associated with this event.
     */
    inline int64_t GetLogdTimestampNs() const { return mLogdTimestampNs; }
    inline int64_t GetElapsedTimestampNs() const { return mElapsedTimestampNs; }

    /**
     * Get the tag for this event.
     */
    inline int GetTagId() const { return mTagId; }

    /**
     * Get the uid of the logging client.
     * Returns -1 if the uid is unknown/has not been set.
     */
    inline int32_t GetUid() const { return mLogUid; }

    /**
     * Get the pid of the logging client.
     * Returns -1 if the pid is unknown/has not been set.
     */
    inline int32_t GetPid() const { return mLogPid; }

    /**
     * Get the nth value, starting at 1.
     *
     * Returns BAD_INDEX if the index is larger than the number of elements.
     * Returns BAD_TYPE if the index is available but the data is the wrong type.
     */
    int64_t GetLong(size_t key, status_t* err) const;
    int GetInt(size_t key, status_t* err) const;
    const char* GetString(size_t key, status_t* err) const;
    bool GetBool(size_t key, status_t* err) const;
    float GetFloat(size_t key, status_t* err) const;

    /**
     * Write test data to the LogEvent. This can only be used when the LogEvent is constructed
     * using LogEvent(tagId, timestampNs). You need to call init() before you can read from it.
     */
    bool write(uint32_t value);
    bool write(int32_t value);
    bool write(uint64_t value);
    bool write(int64_t value);
    bool write(const std::string& value);
    bool write(float value);
    bool write(const std::vector<AttributionNodeInternal>& nodes);
    bool write(const AttributionNodeInternal& node);
    bool writeBytes(const std::string& value);
    bool writeKeyValuePairs(int32_t uid,
                            const std::map<int32_t, int32_t>& int_map,
                            const std::map<int32_t, int64_t>& long_map,
                            const std::map<int32_t, std::string>& string_map,
                            const std::map<int32_t, float>& float_map);

    /**
     * Return a string representation of this event.
     */
    std::string ToString() const;

    /**
     * Write this object to a ProtoOutputStream.
     */
    void ToProto(android::util::ProtoOutputStream& out) const;

    /**
     * Used with the constructor where tag is passed in. Converts the log_event_list to read mode
     * and prepares the list for reading.
     */
    void init();

    /**
     * Set elapsed timestamp if the original timestamp is missing.
     */
    void setElapsedTimestampNs(int64_t timestampNs) {
        mElapsedTimestampNs = timestampNs;
    }

    /**
     * Set the timestamp if the original logd timestamp is missing.
     */
    void setLogdWallClockTimestampNs(int64_t timestampNs) {
        mLogdTimestampNs = timestampNs;
    }

    inline int size() const {
        return mValues.size();
    }

    const std::vector<FieldValue>& getValues() const {
        return mValues;
    }

    std::vector<FieldValue>* getMutableValues() {
        return &mValues;
    }

    bool isValid() {
          return mValid;
    }

    inline LogEvent makeCopy() {
        return LogEvent(*this);
    }

private:
    /**
     * Only use this if copy is absolutely needed.
     */
    LogEvent(const LogEvent&);


    /**
     * Parsing function for new encoding scheme.
     */
    void initNew();

    void parseInt32(int32_t* pos, int32_t depth, bool* last);
    void parseInt64(int32_t* pos, int32_t depth, bool* last);
    void parseString(int32_t* pos, int32_t depth, bool* last);
    void parseFloat(int32_t* pos, int32_t depth, bool* last);
    void parseBool(int32_t* pos, int32_t depth, bool* last);
    void parseByteArray(int32_t* pos, int32_t depth, bool* last);
    void parseKeyValuePairs(int32_t* pos, int32_t depth, bool* last);
    void parseAttributionChain(int32_t* pos, int32_t depth, bool* last);

    /**
     * mBuf is a pointer to the current location in the buffer being parsed.
     * Because the buffer lives  on the StatsSocketListener stack, this pointer
     * is only valid during the LogEvent constructor. It will be set to null at
     * the end of initNew.
     */
    uint8_t* mBuf;

    uint32_t mRemainingLen; // number of valid bytes left in the buffer being parsed
    bool mValid = true; // stores whether the event we received from the socket is valid

    /**
     * Side-effects:
     *    If there is enough space in buffer to read value of type T
     *        - move mBuf past the value that was just read
     *        - decrement mRemainingLen by size of T
     *    Else
     *        - set mValid to false
     */
    template <class T>
    T readNextValue() {
        T value;
        if (mRemainingLen < sizeof(T)) {
            mValid = false;
            value = 0; // all primitive types can successfully cast 0
        } else {
            value = *((T*)mBuf);
            mBuf += sizeof(T);
            mRemainingLen -= sizeof(T);
        }
        return value;
    }

    template <class T>
    void addToValues(int32_t* pos, int32_t depth, T& value, bool* last) {
        Field f = Field(mTagId, pos, depth);
        // do not decorate last position at depth 0
        for (int i = 1; i < depth; i++) {
            if (last[i]) f.decorateLastPos(i);
        }

        Value v = Value(value);
        mValues.push_back(FieldValue(f, v));
    }

    uint8_t getTypeId(uint8_t typeInfo);
    uint8_t getNumAnnotations(uint8_t typeInfo);

    /**
     * Parses a log_msg into a LogEvent object.
     */
    void init(android_log_context context);

    // The items are naturally sorted in DFS order as we read them. this allows us to do fast
    // matching.
    std::vector<FieldValue> mValues;

    // This field is used when statsD wants to create log event object and write fields to it. After
    // calling init() function, this object would be destroyed to save memory usage.
    // When the log event is created from log msg, this field is never initiated.
    android_log_context mContext = NULL;

    // The timestamp set by the logd.
    int64_t mLogdTimestampNs;

    // The elapsed timestamp set by statsd log writer.
    int64_t mElapsedTimestampNs;

    // The atom tag of the event.
    int mTagId;

    // The uid of the logging client (defaults to -1).
    int32_t mLogUid = -1;

    // The pid of the logging client (defaults to -1).
    int32_t mLogPid = -1;
};

void writeExperimentIdsToProto(const std::vector<int64_t>& experimentIds, std::vector<uint8_t>* protoOut);

}  // namespace statsd
}  // namespace os
}  // namespace android

