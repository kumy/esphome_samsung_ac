#pragma once

#include <unity.h>
#include "samsung_proto.h"


namespace esphome
{
    namespace samsung_ac
    {
        void test_add_register_and_build_message()
        {
            SamsungProto::Builder builder;

            // Add registers
            builder.SetMessageType(0x12, 0x04)
                .AddRegister(0x01, {0x0f})
                .AddRegister(0x74, {0xf0});

            // Build message
            SamsungProto message = builder.Build();

            // Check the formatted message
            std::string formatted_message = message.GetFormattedMessage();
            std::string expected_message = "d0c002 12 0000000000 00 fe 1204 06 01010f 7401f0 64 e0";

            // Test that the formatted message matches the expected one
            TEST_ASSERT_EQUAL_STRING(expected_message.c_str(), formatted_message.c_str());
        }

        void test_add_registers_and_build_message()
        {
            SamsungProto::Builder builder;
            builder.SetMessageType(0x1204)
                .AddRegisters({{0x01, {0x0f}},
                               {0x74, {0xf0}}});
            SamsungProto message = builder.Build();

            // Check the formatted message
            std::string formatted_message = message.GetFormattedMessage();
            std::string expected_message = "d0c002 12 0000000000 00 fe 1204 06 01010f 7401f0 64 e0";

            // Test that the formatted message matches the expected one
            TEST_ASSERT_EQUAL_STRING(expected_message.c_str(), formatted_message.c_str());
        }

        void test_calculate_xor_checksum()
        {
            SamsungProto message;

            // Test 1
            std::vector<uint8_t> frame1 = {0xd0, 0xc0, 0x02, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00,
                                           0x00, 0xfe, 0x12, 0x04, 0x06, 0x01, 0x01, 0x0f, 0x74, 0x01,
                                           0xf0, 0x64, 0xe0};
            uint8_t expected_checksum1 = 0x64;
            uint8_t calculated_checksum1 = message.CalculateXorChecksum(frame1);
            TEST_ASSERT_EQUAL_UINT8(expected_checksum1, calculated_checksum1);

            // Test 2
            // d0c002 1c000000000008fe 1302 10 3200 400044004300750076007700780080e0
            std::vector<uint8_t> frame2 = {0xd0, 0xc0, 0x02, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00,
                                           0x08, 0xfe, 0x13, 0x02, 0x10, 0x32, 0x00, 0x40, 0x00,
                                           0x44, 0x00, 0x43, 0x00, 0x75, 0x00, 0x76, 0x00, 0x77, 0x00,
                                           0x78, 0x00, 0x80, 0xe0};
            uint8_t expected_checksum2 = 0x80;
            uint8_t calculated_checksum2 = message.CalculateXorChecksum(frame2);
            TEST_ASSERT_EQUAL_UINT8(expected_checksum2, calculated_checksum2);

            // Test 3
            // d0c002 25 0000000000 b5 fe 1407 19 e00400 01 71 27 e40400071598 e6020573 3902d0b8 f303130709 83 e0
            std::vector<uint8_t> frame3 = {0xd0, 0xc0, 0x02, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00,
                                           0xb5, 0xfe, 0x14, 0x07, 0x19, 0xe0, 0x04, 0x00, 0x01,
                                           0x71, 0x27, 0xe4, 0x04, 0x00, 0x07, 0x15, 0x98, 0xe6, 0x02,
                                           0x05, 0x73, 0x39, 0x02, 0xd0, 0xb8, 0xf3, 0x03, 0x13, 0x07, 0x09, 0x83, 0xe0};
            uint8_t expected_checksum3 = 0x83;
            uint8_t calculated_checksum3 = message.CalculateXorChecksum(frame3);
            TEST_ASSERT_EQUAL_UINT8(expected_checksum3, calculated_checksum3);
        }

        void test_decode_frame1()
        {
            // d0c002 12 0000000000 00 fe 1204 06 01010f 7401f0 64 e0
            std::vector<uint8_t> frame = {
                0xd0, 0xc0, 0x02, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0xfe, 0x12, 0x04, 0x06, 0x01, 0x01, 0x0f, 0x74,
                0x01, 0xf0, 0x64, 0xe0};

            SamsungProto message;
            message.DecodeFrame(frame);

            // Check the message type
            TEST_ASSERT_EQUAL_UINT16(0x1204, message.getMessageType());

            // Check the counter
            TEST_ASSERT_EQUAL_UINT8(0x00, message.getCounter().GetCurrent());

            // Check the number of registers and their length
            TEST_ASSERT_EQUAL_UINT8(2, message.getRegisters().size());
            TEST_ASSERT_EQUAL_UINT8(0x06, message.getPayloadLen());

            // Check the checksum
            TEST_ASSERT_TRUE(message.checkChecksum(0x64));

            // Check the formatted message
            std::string formatted_message = message.GetFormattedMessage();
            std::string expected_message = "d0c002 12 0000000000 00 fe 1204 06 01010f 7401f0 64 e0";
        }

        void test_decode_frame2()
        {
            // d0c002 39 000000000009 fe 1403 2d 3201fe f601fe f403150224 f303130709 f50105 3902d0b8 e00400017127 e40400071598 e801fe e90103 e6020573 1e e0
            std::vector<uint8_t> frame = {
                0xd0, 0xc0, 0x02, 0x39, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x09, 0xfe, 0x14, 0x03, 0x2d, 0x32, 0x01, 0xfe, 0xf6,
                0x01, 0xfe, 0xf4, 0x03, 0x15, 0x02, 0x24, 0xf3, 0x03,
                0x13, 0x07, 0x09, 0xf5, 0x01, 0x05, 0x39, 0x02, 0xd0,
                0xb8, 0xe0, 0x04, 0x00, 0x01, 0x71, 0x27, 0xe4, 0x04,
                0x00, 0x07, 0x15, 0x98, 0xe8, 0x01, 0xfe, 0xe9, 0x01,
                0x03, 0xe6, 0x02, 0x05, 0x73, 0x1e, 0xe0};

            SamsungProto message;
            message.DecodeFrame(frame);

            // Check the message type
            TEST_ASSERT_EQUAL_UINT16(0x1403, message.getMessageType());

            // Check the counter
            TEST_ASSERT_EQUAL_UINT8(0x09, message.getCounter().GetCurrent());

            // Check the number of registers and their length
            TEST_ASSERT_EQUAL_UINT8(11, message.getRegisters().size());
            TEST_ASSERT_EQUAL_UINT8(0x2d, message.getPayloadLen());

            // Check the checksum
            TEST_ASSERT_TRUE(message.checkChecksum(0x1e));

            // Check the formatted message
            std::string formatted_message = message.GetFormattedMessage();
            std::string expected_message = "d0c002 39 000000000009 fe 1403 2d 3201fe f601fe f403150224 f303130709 f50105 3902d0b8 e00400017127 e40400071598 e801fe e90103 e6020573 1e e0";
        }

        // RUNNER

        int TEST_RUN_COUNT = 0;
        int TEST_RUN_MAX = 1;

        int SamsungProtoTestRunner(void)
        {
            UNITY_BEGIN();

            // Runs these tests in all environments, including esphome build.
            RUN_TEST(test_calculate_xor_checksum);
            RUN_TEST(test_add_register_and_build_message);
            RUN_TEST(test_decode_frame1);
            RUN_TEST(test_decode_frame2);

            return UNITY_END();
        }

        int RunSamsungProtoTests(int max = 0)
        {
            if (max > 0)
            {
                TEST_RUN_MAX = max;
            }

            if (TEST_RUN_COUNT < TEST_RUN_MAX)
            {
                TEST_RUN_COUNT += 1;
                return SamsungProtoTestRunner();
            }
            else
            {
                return 1;
            }
        }

    } // esphome
} // samsung_ac
