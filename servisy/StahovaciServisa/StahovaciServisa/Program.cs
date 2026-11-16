using StahovaciServisa.stahovace;
using System;
using System.Xml.Linq;

namespace StahovaciServisa
{
    class Program
    {
        static void Main(string[] args)
        {
            string cesta = @"c:\ctecka\com_zpracovani\vystup.xml";

            Pocasi pocasi = new Pocasi();
            CnbKurzy cnbKurzy = new CnbKurzy();
            AktualniCasProvider aktualniCasProvider = new AktualniCasProvider();

            XDocument doc = new XDocument(
    new XDeclaration("1.0", "utf-8", "yes"),
    new XElement("Zaznamy")
);

            doc.Root.Add(pocasi.ZiskejData());
            doc.Root.Add(cnbKurzy.ZiskejData());
            doc.Root.Add(aktualniCasProvider.ZiskejData());
            doc.Save(cesta);
          
        }
    }
}