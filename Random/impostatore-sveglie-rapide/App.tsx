import React, { useState, useEffect, useCallback } from 'react';
import { AlarmButtonConfig } from './types';
import AlarmButton from './components/AlarmButton';
import CreateButtonModal from './components/CreateButtonModal';
import { PlusIcon, BellIcon, CheckCircleIcon } from './components/icons';

const App: React.FC = () => {
  const [buttons, setButtons] = useState<AlarmButtonConfig[]>(() => {
    try {
      const savedButtons = localStorage.getItem('alarmButtons');
      return savedButtons ? JSON.parse(savedButtons) : [];
    } catch (error) {
      console.error("Failed to parse buttons from localStorage:", error);
      return [];
    }
  });
  const [isModalOpen, setIsModalOpen] = useState(false);
  const [notification, setNotification] = useState<string | null>(null);
  const [notificationType, setNotificationType] = useState<'success' | 'error' | 'info'>('info');


  useEffect(() => {
    localStorage.setItem('alarmButtons', JSON.stringify(buttons));
  }, [buttons]);

  const displayNotification = useCallback((message: string, type: 'success' | 'error' | 'info' = 'info') => {
    setNotification(message);
    setNotificationType(type);
    const timer = setTimeout(() => {
      setNotification(null);
    }, 3000);
    return () => clearTimeout(timer);
  }, []);


  const handleCreateButton = useCallback((name: string, time: string) => {
    const newButton: AlarmButtonConfig = {
      id: crypto.randomUUID(),
      name,
      time,
    };
    setButtons((prevButtons) => [...prevButtons, newButton]);
    setIsModalOpen(false);
    displayNotification(`Pulsante "${name}" creato!`, 'success');
  }, [displayNotification]);

  const handleSetAlarm = useCallback((name: string, time: string) => {
    const now = new Date();
    const nextDay = new Date(now);
    nextDay.setDate(now.getDate() + 1);
    
    const [hours, minutes] = time.split(':');
    nextDay.setHours(parseInt(hours, 10), parseInt(minutes, 10), 0, 0);

    const options: Intl.DateTimeFormatOptions = { weekday: 'long', month: 'long', day: 'numeric' };
    const formattedDate = nextDay.toLocaleDateString('it-IT', options);

    displayNotification(`Sveglia "${name}" impostata per ${formattedDate} alle ${time}.`, 'info');
    // In a real Android app, this would interact with the AlarmManager.
  }, [displayNotification]);

  const handleDeleteButton = useCallback((id: string, name: string) => {
    setButtons(prevButtons => prevButtons.filter(button => button.id !== id));
    displayNotification(`Pulsante "${name}" eliminato.`, 'error');
  }, [displayNotification]);

  const notificationBgColor = {
    success: 'bg-green-600',
    error: 'bg-red-600',
    info: 'bg-sky-600',
  }[notificationType];

  return (
    <div className="min-h-screen bg-slate-900 text-slate-100 flex flex-col items-center p-4 sm:p-6 md:p-8 selection:bg-sky-500 selection:text-white">
      {/* Notification Area */}
      {notification && (
        <div 
          className={`fixed top-5 right-5 ${notificationBgColor} text-white py-3 px-5 rounded-lg shadow-xl z-[100] flex items-center space-x-2 animate-fade-in-down max-w-sm`}
          role="alert"
        >
          <CheckCircleIcon className="w-6 h-6 flex-shrink-0" />
          <span>{notification}</span>
        </div>
      )}
      {/* Inline style for animation as it's simple and component-specific */}
      <style>{`
        @keyframes fade-in-down {
          0% { opacity: 0; transform: translateY(-20px); }
          100% { opacity: 1; transform: translateY(0); }
        }
        .animate-fade-in-down {
          animation: fade-in-down 0.5s ease-out forwards;
        }
      `}</style>

      <header className="w-full max-w-3xl mb-8 text-center">
        <div className="flex items-center justify-center space-x-3 mb-2">
          <BellIcon className="w-10 h-10 text-sky-400" />
          <h1 className="text-4xl font-bold text-sky-400">Impostatore Sveglie Rapide</h1>
        </div>
        <p className="text-slate-400 text-lg">
          Crea pulsanti per impostare velocemente le tue sveglie per il giorno successivo.
        </p>
      </header>

      <main className="w-full max-w-3xl flex-grow">
        <div className="mb-8 flex justify-center">
          <button
            onClick={() => setIsModalOpen(true)}
            className="flex items-center space-x-2 bg-sky-500 hover:bg-sky-600 text-white font-semibold py-3 px-6 rounded-lg shadow-md hover:shadow-lg transition-all duration-200 ease-in-out transform hover:scale-105 focus:outline-none focus:ring-2 focus:ring-sky-400 focus:ring-offset-2 focus:ring-offset-slate-900"
          >
            <PlusIcon className="w-6 h-6" />
            <span>Crea Nuovo Pulsante</span>
          </button>
        </div>

        {buttons.length > 0 ? (
          <div className="grid grid-cols-2 sm:grid-cols-3 md:grid-cols-4 gap-4">
            {buttons.map((button) => (
              <div key={button.id} className="relative group">
                <AlarmButton
                  config={button}
                  onSetAlarm={handleSetAlarm}
                />
                <button
                  onClick={() => handleDeleteButton(button.id, button.name)}
                  className="absolute -top-2 -right-2 bg-red-500 hover:bg-red-600 text-white rounded-full p-1.5 shadow-md opacity-0 group-hover:opacity-100 focus:opacity-100 transition-opacity duration-200 focus:outline-none focus:ring-2 focus:ring-red-400 focus:ring-offset-2 focus:ring-offset-slate-700"
                  aria-label={`Elimina pulsante ${button.name}`}
                >
                  <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={2.5} stroke="currentColor" className="w-4 h-4">
                    <path strokeLinecap="round" strokeLinejoin="round" d="M6 18 18 6M6 6l12 12" />
                  </svg>
                </button>
              </div>
            ))}
          </div>
        ) : (
          <div className="text-center py-10 px-6 bg-slate-800 rounded-lg shadow mt-8">
            <BellIcon className="w-16 h-16 text-slate-500 mx-auto mb-4" />
            <p className="text-slate-400 text-xl mb-2">Nessun pulsante sveglia creato.</p>
            <p className="text-slate-500">Clicca su "Crea Nuovo Pulsante" per iniziare.</p>
          </div>
        )}
      </main>

      <CreateButtonModal
        isOpen={isModalOpen}
        onClose={() => setIsModalOpen(false)}
        onCreate={handleCreateButton}
      />
      
      <footer className="w-full max-w-3xl mt-12 pt-8 border-t border-slate-700 text-center text-slate-500 text-sm">
        <p>&copy; {new Date().getFullYear()} Impostatore Sveglie Rapide. Simula un'app Android.</p>
      </footer>
    </div>
  );
};

export default App;
