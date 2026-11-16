using System;
using System.Collections.Generic;
using System.Text;

namespace StahovaciServisa.stahovace.komponety
{
    public class Konfigurace:IKonfigurace
    {
        public string url { get; set; }
        public string cestaLogu { get; set; }
    }
}
