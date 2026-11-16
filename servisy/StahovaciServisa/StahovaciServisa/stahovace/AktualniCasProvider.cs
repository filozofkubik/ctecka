using System;
using System.Collections.Generic;
using System.Text;
using System.Xml.Linq;

namespace StahovaciServisa.stahovace
{
    public class AktualniCasProvider : IDataProvider
    {

        public string Nazev => "Aktuální čas";


        public XElement ZiskejData()
        {

            string cas = DateTime.Now.ToString("d.M.yyyy HH:mm:ss");


            var element = new XElement("aktualniCas", cas);


            return element;
        }
    }
}
