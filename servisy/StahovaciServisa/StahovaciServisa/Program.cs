using StahovaciServisa.stahovace;
using StahovaciServisa.stahovace.komponety;
using System;
using System.Configuration;
using System.Runtime.CompilerServices;
using System.Xml.Linq;
namespace StahovaciServisa
{
    class Program
    {
        static void Main(string[] args)
        {
            string UrovenLogu = ConfigurationManager.AppSettings["UrovenLogu"];
            string Adresarlogu = ConfigurationManager.AppSettings["Adresarlogu"];
            string AdresarDat = ConfigurationManager.AppSettings["AdresarDat"];
            string AdresacHMU = ConfigurationManager.AppSettings["AdresacHMU"];
            string AdresacNB = ConfigurationManager.AppSettings["AdresacNB"];


            string cesta = AdresarDat+ "\\vystup.xml";

            UrovenLogovani urovenLogovani = (UrovenLogovani)Enum.Parse(typeof(UrovenLogovani), UrovenLogu);



            Pocasi pocasi = new Pocasi(new stahovace.komponety.Konfigurace(AdresacHMU,Adresarlogu, urovenLogovani));
            CnbKurzy cnbKurzy = new CnbKurzy(new stahovace.komponety.Konfigurace(AdresacNB, Adresarlogu, urovenLogovani));
            AktualniCasProvider aktualniCasProvider = new AktualniCasProvider(new stahovace.komponety.Konfigurace("", Adresarlogu, urovenLogovani));

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