namespace KoalaScopeServer
{
    public static class TimedCounter
    {
        public static int CurrentCount { get; private set; } = 0;

        public static void Start(int timeout, FileCollection fc)
        {
            Thread t = new(_ => 
            {
                while (true)
                {
                    CurrentCount++;
                    if (CurrentCount >= fc.GetFileCount()) CurrentCount = 0;
                    Thread.Sleep(timeout);
                }
            }); 
            t.Start();                       
        }
    }
}
