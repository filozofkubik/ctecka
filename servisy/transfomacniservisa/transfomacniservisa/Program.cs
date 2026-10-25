using System;
using transfomacniservisa;

namespace MojeAplikace
{
    internal class Program
    {
        static string cestadoctecky = @"c:\ctecka\com_ctecka\";
        static string cestazpracovani = @"c:\ctecka\com_zpracovani\";
        static int timer = 3000;
        static void Main(string[] args)
        {
            
            List<ObrazBasic> transformace = new List<ObrazBasic>();

            Obraz005 x = new ();
            x.Inicializuj(cestazpracovani, cestadoctecky, "obraz005.txt");
          
            transformace.Add(x);

            Console.WriteLine("Aplikace spuštěna!");
            while (true)
            {foreach (ObrazBasic t in transformace)
                {
                    Thread.Sleep(timer);
                   
                    t.ZpracujSoubor();
                    Console.WriteLine("Zpracování dat");
                }
                
            }
          
            Console.ReadLine();
        }
    }
}