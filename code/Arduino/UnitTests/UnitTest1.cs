using System;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace UnitTests
{
    [TestClass]
    public class UnitTest1
    {
        const int AverageCount = 100;

        UInt32 _movingAverageBatteryVolt;
        UInt32 _movingAverageSumBatteryVolt;

        [TestMethod]
        public void TestMethod1()
        {
            _movingAverageSumBatteryVolt = 65536;
            _movingAverageBatteryVolt = 0;
            var r = new Random(555);
            for (int i = 0; i < AverageCount * 10; i++)
            {
                _movingAverageSumBatteryVolt = _movingAverageSumBatteryVolt - _movingAverageBatteryVolt;
                _movingAverageSumBatteryVolt = _movingAverageSumBatteryVolt + (UInt32)r.Next(488, 525);
                _movingAverageBatteryVolt = _movingAverageSumBatteryVolt / AverageCount;
                Console.WriteLine(_movingAverageBatteryVolt);
            }
        }
    }
}
