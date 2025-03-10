#pragma once

#include <vector>
#include <stdint.h>
#include <stdexcept>

// #if IS_NATIVE == 1
// #include <ArduinoFake.h>
// #else
// #include <Arduino.h>
// #endif

namespace esphome
{
    namespace samsung_ac
    {

        class Counter
        {
        public:
            Counter(uint8_t initial_value = 0);

            uint8_t GetCurrent() const;
            uint8_t GetNext();
            void SetNext(uint8_t value);
            void Set(uint8_t value);

        private:
            uint8_t current_value;
        };

        class SamsungProto
        {
        public:
            struct Register
            {
                uint8_t id;
                uint8_t len;
                std::vector<uint8_t> value;
            };

            struct Payload
            {
                uint8_t messageType[2];
                uint8_t payloadLen;
                std::vector<Register> registers;
            };

            SamsungProto();

            bool ValidateFrame(const std::vector<uint8_t> &frame) const;
            void DecodeFrame(const std::vector<uint8_t> &frame);
            void DecodeFrame(const uint8_t *data, size_t len);
            std::vector<uint8_t> GetFullMessageBytes() const;
            std::string FormatHexPretty(const std::vector<uint8_t> &data) const;
            std::string GetFormattedMessage() const;
            uint8_t GetRegisterValue(uint8_t registerId) const;
            std::vector<Register>::const_iterator getRegisterIterator() const;
            std::vector<Register>::const_iterator getRegisterEndIterator() const;
            bool IsAck() const;
            SamsungProto Ack() const;
            bool HasRegister(uint8_t registerId) const;
            uint16_t getMessageType() const;
            bool isMessageType(uint8_t msgType) const;
            bool isMessageType(uint16_t msgType) const;
            uint8_t getMessageLen() const;
            uint8_t getPayloadLen() const;
            const std::vector<Register> &getRegisters() const;
            Counter getCounter() const;
            bool checkChecksum(uint8_t expectedChecksum) const;
            uint8_t CalculateXorChecksum(const std::vector<uint8_t> &frame) const;

            std::string format_hex_pretty(uint8_t byte) const;
            std::string format_hex_pretty(const std::vector<uint8_t> &data) const;

            // Builder pattern (simplified)
            class Builder
            {
            public:
                Builder &SetMessageType(uint8_t byte1, uint8_t byte2);
                Builder &SetMessageType(uint16_t type);
                Builder &AddRegister(uint8_t regId, const std::vector<uint8_t> &value);
                // Builder &AddRegisters(const std::vector<Register> &registers);
                Builder &AddRegisters(const std::vector<std::pair<uint8_t, std::vector<uint8_t>>> &registers);
                SamsungProto Build();

            private:
                uint8_t messageType[2];
                uint8_t payloadLen = 0;
                std::vector<Register> registers;
            };

            class RegisterBuilder
            {
            public:
                RegisterBuilder &SetId(uint8_t id);
                RegisterBuilder &SetValue(const std::vector<uint8_t> &value);
                Register Build() const;

            private:
                Register register_;
            };

        private:
            uint8_t messageLen;
            Payload payload;
            uint8_t checksum;
            uint8_t footer;
            Counter counter;
        };

    } // namespace samsung_ac
} // namespace esphome

//  d0c0023 00000000000 06 fe 1206 24 0201f0 410132 4301e2 440112 620100 6301c2 ea01fe 5a0115 5c0117 730100 f70400000000 cf e0