using System;
using System.Collections.Generic;
using System.Text;
using StahovaciServisa.stahovace.komponety;

namespace StahovaciServisa.stahovace.komponety
{
    public class Konfigurace:IKonfigurace
    {
        public string url { get;  }
        public string chybovyadresar { get;  }
        public UrovenLogovani urovenLogovani { get;  }

        public Konfigurace(string url, string chybovyadresar, UrovenLogovani urovenLogovani)
        {
            this.url = url;
            this.chybovyadresar = chybovyadresar;
            this.urovenLogovani = urovenLogovani;
        }
    }

}
