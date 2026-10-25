using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace transfomacniservisa
{
    public class Obraz005 : ObrazBasic
    { 
        public override void ZpracujSoubor()
        {
            // Implementace metody ZpracujSoubor pro třídu Obraz005
            StreamWriter writer = new StreamWriter(System.IO.Path.Combine(nazevVystupnihoadreare, nazevSouboru));
            writer.WriteLine("20; 20; 10; red; Pravidla bezpečného zacházení se čtečkou;");
            writer.WriteLine("20; 20; 10; red; Nikdy nechávej čtečku bez dozoru;");
            writer.WriteLine("60; 60; 10; black; Nesahej na ni se špinavýma rukama;");
            writer.Close();
            Console.WriteLine($"Zpracovávám soubor {nazevSouboru} z adresáře {nazevVstupnihoadresare} a ukládám do {nazevVystupnihoadreare}");
            
        }
    }
}
