import React from 'react';
import { AlarmButtonConfig } from '../types';
import { AlarmIcon } from './icons';

interface AlarmButtonProps {
  config: AlarmButtonConfig;
  onSetAlarm: (name: string, time: string) => void;
}

const AlarmButton: React.FC<AlarmButtonProps> = ({ config, onSetAlarm }) => {
  return (
    <button
      onClick={() => onSetAlarm(config.name, config.time)}
      className="flex flex-col items-center justify-center p-4 bg-slate-700 hover:bg-slate-600 text-slate-100 rounded-xl shadow-lg transition-all duration-200 ease-in-out transform hover:scale-105 focus:outline-none focus:ring-2 focus:ring-sky-400 focus:ring-opacity-75 w-full aspect-square"
    >
      <AlarmIcon className="w-10 h-10 mb-2 text-sky-400" />
      <span className="text-sm font-medium text-center truncate w-full px-1">{config.name}</span>
      <span className="text-xs text-slate-400">{config.time}</span>
    </button>
  );
};

export default AlarmButton;
