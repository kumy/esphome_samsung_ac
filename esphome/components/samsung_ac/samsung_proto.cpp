
#include "esphome/core/log.h"
#include "samsung_proto.h"

namespace esphome
{
    namespace samsung_ac
    {

        Counter::Counter(uint8_t initial_value) : current_value(initial_value) {}

        uint8_t Counter::GetCurrent() const
        {
            return this->current_value;
        }

        uint8_t Counter::GetNext()
        {
            return ++this->current_value;
        }

        void Counter::SetNext(uint8_t value)
        {
            this->current_value = value;
        }

        void Counter::Set(uint8_t value)
        {
            this->current_value = value;
        }

        // ---------------------------------------------
        // SamsungProto Implementation
        // ---------------------------------------------

        SamsungProto::SamsungProto() : messageLen(0), checksum(0), footer(0xe0), counter(0) {}

        bool SamsungProto::ValidateFrame(const std::vector<uint8_t> &frame) const
        {
            // Ensure frame size is large enough (the minimal frame size is 14 + payloadLen + checksum + footer)
            if (frame.size() < 14) // Minimum 14 bytes for header and static parts (before the payload)
            {

                return false;
            }

            // Extract message length from the 13th byte (frame[13])
            size_t payloadLen = frame[13];

            // Calculate expected frame size: static part (14 bytes) + payload length + checksum + footer
            size_t expectedSize = 14 + payloadLen + 1 + 1; // +1 for checksum, +1 for footer

            if (frame.size() != expectedSize)
            {
                ESP_LOGW("SamsungProto", "ValidateFrame0: %d != %d", frame.size() - 1, expectedSize);
                return false;
            }

            uint8_t calc_checksum = this->CalculateXorChecksum(frame);
            if (frame[frame.size() - 2] < calc_checksum) // The checksum is the second-to-last byte before the footer
            {
                ESP_LOGW("SamsungProto", "ValidateFrame1");
                return false;
            }

            if (frame[0] != 0xd0 || frame[1] != 0xc0 || frame[2] != 0x02)
            {
                ESP_LOGW("SamsungProto", "ValidateFrame2");
                return false;
            }

            if (frame[4] != 0x00 || frame[5] != 0x00 || frame[6] != 0x00 || frame[7] != 0x00 || frame[8] != 0x00)
            {
                ESP_LOGW("SamsungProto", "ValidateFrame3");
                return false;
            }

            if (frame[10] != 0xfe)
            {
                ESP_LOGW("SamsungProto", "ValidateFrame4");
                return false;
            }

            if (frame[frame.size() - 1] != 0xe0)
            {
                ESP_LOGW("SamsungProto", "ValidateFrame5: %02x", frame[frame.size() - 1]);
                return false;
            }

            return true;
        }

        void SamsungProto::DecodeFrame(const uint8_t *data, size_t len)
        {
            std::vector<uint8_t> frame(data, data + len);
            this->DecodeFrame(frame);
        }

        void SamsungProto::DecodeFrame(const std::vector<uint8_t> &frame)
        {
            // First validate the frame
            if (!this->ValidateFrame(frame))
            {
                // You can log the error or handle it as needed, for example:
                // Serial.println("Invalid frame received.");
                return;
            }

            if (frame.size() < 11)
            { // Minimal size of the frame is 11 bytes
                return;
                // throw std::invalid_argument("Frame is too small.");
            }

            // Ensure header matches
            if (frame[0] != 0xd0 || frame[1] != 0xc0 || frame[2] != 0x02)
            {
                return;
                // throw std::invalid_argument("Invalid header.");
            }

            this->messageLen = frame[3];

            if (frame[4] != 0x00 || frame[5] != 0x00 || frame[6] != 0x00 || frame[7] != 0x00 || frame[8] != 0x00)
            {
                return;
                // throw std::invalid_argument("Invalid static bytes.");
            }

            // Get the counter
            this->counter.Set(frame[9]);

            if (frame[10] != 0xfe)
            {
                return;
                // throw std::invalid_argument("Invalid separator.");
            }

            // Now parsing the payload
            this->payload.messageType[0] = frame[11];
            this->payload.messageType[1] = frame[12];
            this->payload.payloadLen = frame[13];

            size_t index = 14;
            this->payload.registers.clear();
            while (index < frame.size() - 3)
            {
                Register reg;
                reg.id = frame[index];
                reg.len = frame[index + 1];
                reg.value.resize(reg.len);
                std::copy(frame.begin() + index + 2, frame.begin() + index + 2 + reg.len, reg.value.begin());
                this->payload.registers.push_back(reg);
                index += 2 + reg.len;
            }

            this->checksum = frame[index];
            this->footer = frame[index + 1];

            // // Verify checksum
            // uint8_t calc_checksum = CalculateXorChecksum(frame);
            // if (checksum != calc_checksum)
            // {
            //     // std::ostringstream oss;
            //     // oss << "Invalid checksum: 0x" << std::hex << (int)calc_checksum
            //     //     << ", Actual: 0x" << std::hex << (int)checksum;

            //     // throw std::invalid_argument(oss.str());
            //     // throw std::invalid_argument("Invalid checksum");
            // }

            // // Ensure footer is correct
            // if (footer != 0xe0)
            // {
            //     // throw std::invalid_argument("Invalid footer.");
            // }
        }

        std::vector<uint8_t> SamsungProto::GetFullMessageBytes() const
        {
            std::vector<uint8_t> frame;

            // Add static header
            frame.push_back(0xd0);
            frame.push_back(0xc0);
            frame.push_back(0x02);

            // Add message length
            frame.push_back(messageLen);

            // Add static bytes
            frame.push_back(0x00);
            frame.push_back(0x00);
            frame.push_back(0x00);
            frame.push_back(0x00);
            frame.push_back(0x00);

            // Add counter
            frame.push_back(this->counter.GetCurrent());

            // Add separator
            frame.push_back(0xfe);

            // Add message type
            frame.push_back(this->payload.messageType[0]);
            frame.push_back(this->payload.messageType[1]);

            // Add payload length
            frame.push_back(this->payload.payloadLen);

            // Add registers
            for (const Register &reg : this->payload.registers)
            {
                frame.push_back(reg.id);
                frame.push_back(reg.len);
                frame.insert(frame.end(), reg.value.begin(), reg.value.end());
            }

            // Add checksum (it will be calculated)
            uint8_t calc_checksum = this->CalculateXorChecksum(frame);
            frame.push_back(calc_checksum);

            // Add footer
            frame.push_back(footer);

            return frame;
        }
        std::string SamsungProto::format_hex_pretty(uint8_t byte) const
        {
            char buffer[5];
            snprintf(buffer, sizeof(buffer), "%02x", byte);
            return std::string(buffer);
        }
        std::string SamsungProto::format_hex_pretty(const std::vector<uint8_t> &data) const
        {
            std::string result;
            for (uint8_t byte : data)
            {
                result += format_hex_pretty(byte);
            }
            return result;
        }
        std::string SamsungProto::FormatHexPretty(const std::vector<uint8_t> &data) const
        {
            std::string result = "";

            size_t i = 0;

            // First fixed header part "d0c002"
            result += this->format_hex_pretty(data[i++]);
            result += this->format_hex_pretty(data[i++]);
            result += this->format_hex_pretty(data[i++]);
            result += " "; // Space after the header part

            // Message length byte
            result += this->format_hex_pretty(data[i++]);
            result += " ";

            // Static zero bytes (0x00), exactly 5 zero bytes
            for (int j = 0; j < 5; ++j)
            {
                result += this->format_hex_pretty(data[i++]);
            }
            result += " ";

            // Message counter byte
            result += this->format_hex_pretty(data[i++]);
            result += " ";

            // Static byte 0xFE separator
            result += this->format_hex_pretty(data[i++]);
            result += " ";

            // Message type (2 bytes)
            result += this->format_hex_pretty(data[i++]);
            result += this->format_hex_pretty(data[i++]);
            result += " ";

            // Payload length byte (1 byte)
            result += this->format_hex_pretty(data[i++]);
            result += " ";

            // Payload registers (grouped as space-separated continuous byte sequences)
            while (i < data.size() - 2)
            { // Stop before the checksum and footer
                // Register group: id + len + value
                result += this->format_hex_pretty(data[i++]); // Register id
                result += this->format_hex_pretty(data[i++]); // Register len
                size_t len = data[i - 1];                     // len determines how many bytes to take from value

                for (size_t j = 0; j < len; ++j)
                {
                    result += this->format_hex_pretty(data[i++]);
                }

                result += " "; // Separate register groups with space
            }

            // Checksum (1 byte)
            result += this->format_hex_pretty(data[i++]);
            result += " ";

            // Footer byte (0xe0)
            result += this->format_hex_pretty(data[i++]);

            return result;
        }

        std::string SamsungProto::GetFormattedMessage() const
        {
            return this->FormatHexPretty(this->GetFullMessageBytes());
        }

        bool SamsungProto::HasRegister(uint8_t registerId) const
        {
            for (const Register &reg : this->payload.registers)
            {
                if (reg.id == registerId)
                {
                    return true;
                }
            }
            return false;
        }

        uint8_t SamsungProto::GetRegisterValue(uint8_t registerId) const
        {
            for (const Register &reg : this->payload.registers)
            {
                if (reg.id == registerId)
                {
                    return reg.value.empty() ? 0 : reg.value[0];
                }
            }
            return 0x00;
            // throw std::out_of_range("Register not found.");
        }

        std::vector<SamsungProto::Register>::const_iterator SamsungProto::getRegisterIterator() const
        {
            return this->payload.registers.begin();
        }

        std::vector<SamsungProto::Register>::const_iterator SamsungProto::getRegisterEndIterator() const
        {
            return this->payload.registers.end();
        }

        bool SamsungProto::IsAck() const
        {
            return (payload.messageType[1] % 2 == 1); // If the 2nd byte is odd, it's an ACK
        }

        SamsungProto SamsungProto::Ack() const
        {
            Builder builder = Builder();
            builder.SetMessageType(this->getMessageType() + 0x01);
            for (auto it = this->getRegisterIterator(); it != this->getRegisterEndIterator(); ++it)
            {
                const SamsungProto::Register &reg = *it;
                builder.AddRegister(reg.id, reg.value);
            }
            return builder.Build();
        }

        // XOR Checksum Calculation
        uint8_t SamsungProto::CalculateXorChecksum(const std::vector<uint8_t> &frame) const
        {
            // d0c002 12 0000000000 00 fe 1204 06 01010f 7401f0 64 e0
            // d0c002 12 0000000000 00 fe 1204 06 01010f 7401f0 94 e0
            // if (frame.size() < 15)
            // {
            //     return nullptr;
            //     // throw std::invalid_argument("Frame size is too small.");
            // }
            size_t payloadLen = frame[13];

            // if (payloadLen < 1)
            // {
            //     return nullptr;
            //     // throw std::invalid_argument("Invalid payload size.");
            // }

            uint8_t checksum = 0x00;
            // std::ostringstream oss;
            // oss << "payload len:" << std::hex << (int)payloadLen;
            for (size_t i = 0; i < 14 + payloadLen; ++i)
            {
                // oss << " " << std::hex << (int)frame[i];
                checksum ^= frame[i];
            }

            // throw std::invalid_argument(oss.str());
            return checksum;
        }

        // ---------------------------------------------
        // Builder Implementation
        // ---------------------------------------------

        SamsungProto::Builder &SamsungProto::Builder::SetMessageType(uint8_t byte1, uint8_t byte2)
        {
            this->messageType[0] = byte1;
            this->messageType[1] = byte2;
            return *this;
        }
        SamsungProto::Builder &SamsungProto::Builder::SetMessageType(uint16_t type)
        {
            this->messageType[0] = static_cast<uint8_t>((type >> 8) & 0xFF);
            this->messageType[1] = static_cast<uint8_t>(type & 0xFF);
            return *this;
        }
        SamsungProto::Builder &SamsungProto::Builder::AddRegister(uint8_t regId, const std::vector<uint8_t> &value)
        {
            Register reg;
            reg.id = regId;
            reg.len = value.size(); // Automatically set the length based on the value size
            reg.value = value;
            registers.push_back(reg);

            // Update payload length based on this register (2 bytes for the register header + value size)
            payloadLen += 2 + reg.len;
            return *this;
        }

        // SamsungProto::Builder &SamsungProto::Builder::AddRegisters(const std::vector<Register> &registers)
        // {
        //     this->registers.clear();

        //     this->payloadLen = 0;
        //     for (const auto &reg : registers)
        //     {
        //         this->registers.push_back(reg);
        //         this->payloadLen += reg.len;
        //     }

        //     return *this;
        // }
        SamsungProto::Builder &SamsungProto::Builder::AddRegisters(const std::vector<std::pair<uint8_t, std::vector<uint8_t>>> &registers)
        {
            this->registers.clear();
            for (const auto &reg : registers)
            {
                this->registers.push_back(RegisterBuilder()
                                              .SetId(reg.first)
                                              .SetValue(reg.second)
                                              .Build());
            }

            // Recalculate the payload length based on the registers
            this->payloadLen = 0;
            for (const auto &reg : this->registers)
            {
                this->payloadLen += 2 + reg.len;
            }

            return *this;
        }
        SamsungProto SamsungProto::Builder::Build()
        {
            SamsungProto proto;
            proto.payload.messageType[0] = messageType[0];
            proto.payload.messageType[1] = messageType[1];
            proto.payload.payloadLen = payloadLen;
            proto.payload.registers = registers;
            proto.messageLen = 12 + payloadLen;
            std::vector<uint8_t> frame = proto.GetFullMessageBytes();
            proto.checksum = proto.CalculateXorChecksum(frame);
            proto.footer = 0xe0;
            return proto;
        }

        uint16_t SamsungProto::getMessageType() const
        {
            return (static_cast<uint16_t>(this->payload.messageType[0]) << 8) | static_cast<uint16_t>(this->payload.messageType[1]);
        }

        bool SamsungProto::isMessageType(uint8_t msgType) const
        {
            return this->payload.messageType[0] == msgType;
        }

        bool SamsungProto::isMessageType(uint16_t msgType) const
        {
            return this->getMessageType() == msgType;
        }

        uint8_t SamsungProto::getMessageLen() const
        {
            return 4 + this->messageLen;
        }

        uint8_t SamsungProto::getPayloadLen() const
        {
            return this->payload.payloadLen;
        }

        const std::vector<SamsungProto::Register> &SamsungProto::getRegisters() const
        {
            return this->payload.registers;
        }
        Counter SamsungProto::getCounter() const
        {
            return this->counter;
        }
        bool SamsungProto::checkChecksum(uint8_t expectedChecksum) const
        {
            return (this->checksum == expectedChecksum);
        }

        SamsungProto::RegisterBuilder &SamsungProto::RegisterBuilder::SetId(uint8_t id)
        {
            this->register_.id = id;
            return *this;
        }

        // Set the register value
        SamsungProto::RegisterBuilder &SamsungProto::RegisterBuilder::SetValue(const std::vector<uint8_t> &value)
        {
            this->register_.value = value;
            this->register_.len = value.size(); // Automatically set the length based on the value size
            return *this;
        }

        // Build and return the Register object
        SamsungProto::Register SamsungProto::RegisterBuilder::Build() const
        {
            return this->register_;
        }

    } // namespace samsung_ac
} // namespace esphome
