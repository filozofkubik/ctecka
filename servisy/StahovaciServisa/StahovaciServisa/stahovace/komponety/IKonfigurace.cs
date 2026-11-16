using System;
using System.Collections.Generic;
using System.Text;

namespace StahovaciServisa.stahovace.komponety
{
    public interface IKonfigurace
    {
        string chybovyadresar { get; }
        UrovenLogovani urovenLogovani { get; }
    }

}
