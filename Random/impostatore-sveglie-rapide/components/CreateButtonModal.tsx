
import React, { useState, useEffect } from 'react';

interface CreateButtonModalProps {
  isOpen: boolean;
  onClose: () => void;
  onCreate: (name: string, time: string) => void;
}

const CreateButtonModal: React.FC<CreateButtonModalProps> = ({ isOpen, onClose, onCreate }) => {
  const [name, setName] = useState('');
  const [time, setTime] = useState('');
  const [error, setError] = useState('');

  useEffect(() => {
    if (isOpen) {
      // Reset form when modal opens
      setName('');
      setTime('');
      setError('');
    }
  }, [isOpen]);

  const handleSubmit = () => {
    if (!name.trim()) {
      setError('Il nome del pulsante non può essere vuoto.');
      return;
    }
    if (!time) {
      setError('Per favore, imposta un orario.');
      return;
    }
    setError('');
    onCreate(name.trim(), time);
  };
  
  // Add keydown listener for Escape key to close modal
  useEffect(() => {
    const handleEsc = (event: KeyboardEvent) => {
      if (event.key === 'Escape') {
        onClose();
      }
    };
    // We only add/remove the event listener if the modal is open,
    // but the hook itself is called on every render.
    if (isOpen) {
      window.addEventListener('keydown', handleEsc);
    }
    return () => {
      window.removeEventListener('keydown', handleEsc);
    };
  }, [isOpen, onClose]);

  // All hooks have been called. Now we can conditionally return null.
  if (!isOpen) {
    return null;
  }

  return (
    <div 
        className="fixed inset-0 bg-black bg-opacity-75 flex items-center justify-center p-4 z-50 transition-opacity duration-300 ease-in-out" 
        onClick={onClose}
        role="dialog"
        aria-modal="true"
        aria-labelledby="modal-title"
    >
      <div 
        className="bg-slate-800 p-6 rounded-lg shadow-xl w-full max-w-md transform transition-all duration-300 ease-in-out scale-95 opacity-0 animate-modal-appear"
        style={{ animationName: 'modalAppear', animationDuration: '0.3s', animationFillMode: 'forwards' }}
        onClick={(e) => e.stopPropagation()} // Prevent click inside modal from closing it
      >
        <h2 id="modal-title" className="text-2xl font-semibold mb-6 text-sky-400">Crea Nuovo Pulsante Sveglia</h2>
        
        {error && <p className="text-red-400 text-sm mb-4 bg-red-900/30 p-2 rounded-md">{error}</p>}

        <div className="mb-4">
          <label htmlFor="buttonName" className="block text-sm font-medium text-slate-300 mb-1">
            Nome del Pulsante
          </label>
          <input
            type="text"
            id="buttonName"
            value={name}
            onChange={(e) => setName(e.target.value)}
            placeholder="Es. Sveglia Mattutina"
            className="w-full p-3 bg-slate-700 border border-slate-600 rounded-md text-slate-100 placeholder-slate-500 focus:ring-2 focus:ring-sky-500 focus:border-sky-500 outline-none"
            autoFocus
          />
        </div>

        <div className="mb-6">
          <label htmlFor="alarmTime" className="block text-sm font-medium text-slate-300 mb-1">
            Orario Sveglia
          </label>
          <input
            type="time"
            id="alarmTime"
            value={time}
            onChange={(e) => setTime(e.target.value)}
            className="w-full p-3 bg-slate-700 border border-slate-600 rounded-md text-slate-100 focus:ring-2 focus:ring-sky-500 focus:border-sky-500 outline-none appearance-none"
            // Apply specific styling for time input text color in supporting browsers
            style={{ colorScheme: 'dark' }} 
          />
        </div>

        <div className="flex justify-end space-x-3">
          <button
            onClick={onClose}
            type="button"
            className="px-5 py-2.5 text-sm font-medium text-slate-300 bg-slate-700 hover:bg-slate-600 rounded-lg focus:outline-none focus:ring-2 focus:ring-slate-500 transition-colors"
          >
            Annulla
          </button>
          <button
            onClick={handleSubmit}
            type="button"
            className="px-5 py-2.5 text-sm font-medium text-white bg-sky-500 hover:bg-sky-600 rounded-lg focus:outline-none focus:ring-2 focus:ring-sky-400 focus:ring-offset-2 focus:ring-offset-slate-800 transition-colors"
          >
            Crea Pulsante
          </button>
        </div>
      </div>
      {/* Using inline style for keyframes as per Tailwind's suggestion for one-off animations */}
      <style>{`
        @keyframes modalAppear {
          from { opacity: 0; transform: scale(0.95) translateY(10px); }
          to { opacity: 1; transform: scale(1) translateY(0); }
        }
      `}</style>
    </div>
  );
};

export default CreateButtonModal;
