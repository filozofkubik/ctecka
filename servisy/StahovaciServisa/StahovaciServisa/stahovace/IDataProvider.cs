using StahovaciServisa.stahovace.komponety;
using System;
using System.Collections.Generic;
using System.Text;
using System.Xml.Linq;

namespace StahovaciServisa.stahovace

{
    public interface IDataProvider
    {


     
        string Nazev { get; }



        XElement ZiskejData();
    }
}
