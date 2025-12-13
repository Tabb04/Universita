import React, { useState, useEffect } from 'react';
import { Search, Star, Film, AlertCircle, Loader2, Settings, Key, Info } from 'lucide-react';

// --- COSTANTI E UTILITIES ---

const RATING_COLORS = [
  { min: 9.0, color: 'bg-emerald-500 text-white', label: 'Capolavoro (9.0+)' },
  { min: 8.0, color: 'bg-green-500 text-white', label: 'Ottimo (8.0-8.9)' },
  { min: 7.0, color: 'bg-yellow-400 text-black', label: 'Buono (7.0-7.9)' },
  { min: 6.0, color: 'bg-orange-400 text-black', label: 'Regolare (6.0-6.9)' },
  { min: 0.0, color: 'bg-red-500 text-white', label: 'Scarso (<6.0)' },
];

const getRatingColor = (rating) => {
  if (!rating || rating === 'N/A') return 'bg-slate-800 text-slate-600';
  const numRating = parseFloat(rating);
  const match = RATING_COLORS.find(c => numRating >= c.min);
  return match ? match.color : 'bg-slate-800';
};

// --- COMPONENTE PRINCIPALE ---

const SeriesRatingsApp = () => {
  const [query, setQuery] = useState('');
  const [dataSource, setDataSource] = useState('tvmaze'); // 'tvmaze' | 'omdb'
  const [omdbKey, setOmdbKey] = useState('');
  const [seriesData, setSeriesData] = useState(null);
  const [episodes, setEpisodes] = useState({});
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState(null);
  const [maxEpisodes, setMaxEpisodes] = useState(0);
  const [progress, setProgress] = useState('');

  // Carica la chiave salvata all'avvio
  useEffect(() => {
    const savedKey = localStorage.getItem('omdb_api_key');
    if (savedKey) setOmdbKey(savedKey);
  }, []);

  // Salva la chiave quando cambia
  const handleKeyChange = (e) => {
    const newKey = e.target.value;
    setOmdbKey(newKey);
    localStorage.setItem('omdb_api_key', newKey);
  };

  const fetchTVMaze = async () => {
    setProgress('Ricerca serie su TVMaze...');
    // 1. Cerca la serie
    const searchRes = await fetch(`https://api.tvmaze.com/search/shows?q=${encodeURIComponent(query)}`);
    const searchData = await searchRes.json();

    if (!searchData || searchData.length === 0) {
      throw new Error("Serie non trovata su TVMaze.");
    }

    const show = searchData[0].show;
    
    setProgress('Scaricamento episodi...');
    // 2. Ottieni episodi
    const episodesRes = await fetch(`https://api.tvmaze.com/shows/${show.id}/episodes`);
    const episodesData = await episodesRes.json();

    // 3. Normalizza dati
    const episodesBySeason = {};
    let maxEp = 0;

    episodesData.forEach(ep => {
      const s = ep.season;
      if (!episodesBySeason[s]) episodesBySeason[s] = [];
      episodesBySeason[s].push({
        season: ep.season,
        number: ep.number,
        title: ep.name,
        rating: ep.rating?.average || 0,
        id: ep.id
      });
    });

    // Calcola max episodi per riga
    Object.values(episodesBySeason).forEach(list => {
      if (list.length > maxEp) maxEp = list.length;
    });

    return {
      show: {
        name: show.name,
        image: show.image?.original,
        rating: show.rating?.average,
        year: show.premiered?.split('-')[0],
        genres: show.genres,
        summary: show.summary,
        source: 'TVMaze'
      },
      episodes: episodesBySeason,
      maxEpisodes: maxEp
    };
  };

  const fetchOMDb = async () => {
    if (!omdbKey) throw new Error("Inserisci una API Key valida per usare OMDb.");

    setProgress('Ricerca serie su OMDb...');
    // 1. Cerca metadati serie
    const baseRes = await fetch(`https://www.omdbapi.com/?apikey=${omdbKey}&t=${encodeURIComponent(query)}&type=series`);
    const baseData = await baseRes.json();

    if (baseData.Response === 'False') {
      throw new Error(baseData.Error || "Serie non trovata su OMDb.");
    }

    const totalSeasons = parseInt(baseData.totalSeasons);
    const imdbID = baseData.imdbID;
    const episodesBySeason = {};
    let maxEp = 0;

    // 2. Itera per ogni stagione (OMDb richiede una chiamata per stagione)
    for (let i = 1; i <= totalSeasons; i++) {
      setProgress(`Scaricamento Stagione ${i} di ${totalSeasons}...`);
      const seasonRes = await fetch(`https://www.omdbapi.com/?apikey=${omdbKey}&i=${imdbID}&Season=${i}`);
      const seasonData = await seasonRes.json();

      if (seasonData.Response === 'True' && seasonData.Episodes) {
        episodesBySeason[i] = seasonData.Episodes.map(ep => ({
          season: i,
          number: parseInt(ep.Episode),
          title: ep.Title,
          rating: ep.imdbRating !== 'N/A' ? parseFloat(ep.imdbRating) : 0,
          id: ep.imdbID
        }));

        if (seasonData.Episodes.length > maxEp) maxEp = seasonData.Episodes.length;
      }
    }

    return {
      show: {
        name: baseData.Title,
        image: baseData.Poster !== 'N/A' ? baseData.Poster : null,
        rating: baseData.imdbRating,
        year: baseData.Year?.split('–')[0],
        genres: baseData.Genre?.split(', '),
        summary: baseData.Plot,
        source: 'IMDb (via OMDb)'
      },
      episodes: episodesBySeason,
      maxEpisodes: maxEp
    };
  };

  const handleSearch = async (e) => {
    e.preventDefault();
    if (!query.trim()) return;

    setLoading(true);
    setError(null);
    setSeriesData(null);
    setEpisodes({});
    setProgress('');

    try {
      let data;
      if (dataSource === 'omdb') {
        data = await fetchOMDb();
      } else {
        data = await fetchTVMaze();
      }

      setSeriesData(data.show);
      setEpisodes(data.episodes);
      setMaxEpisodes(data.maxEpisodes);
    } catch (err) {
      setError(err.message);
    } finally {
      setLoading(false);
      setProgress('');
    }
  };

  const range = (n) => Array.from({ length: n }, (_, i) => i + 1);

  return (
    <div className="min-h-screen bg-slate-950 text-slate-100 font-sans p-4 md:p-8">
      
      {/* Header */}
      <div className="max-w-7xl mx-auto mb-8">
        <h1 className="text-3xl font-bold mb-6 flex items-center gap-2 text-emerald-400">
          <Film className="w-8 h-8" />
          Series Graph
        </h1>
        
        {/* Pannello Controlli */}
        <div className="bg-slate-900 p-6 rounded-xl border border-slate-800 mb-6 shadow-xl">
          <form onSubmit={handleSearch} className="space-y-4">
            
            {/* Input Ricerca */}
            <div className="flex gap-2">
              <div className="relative flex-1">
                <input
                  type="text"
                  placeholder="Cerca serie (es. Breaking Bad, The Office)..."
                  value={query}
                  onChange={(e) => setQuery(e.target.value)}
                  className="w-full bg-slate-800 border border-slate-700 rounded-lg py-3 px-4 pl-10 focus:outline-none focus:ring-2 focus:ring-emerald-500 transition-all text-white placeholder-slate-400"
                />
                <Search className="absolute left-3 top-3.5 w-5 h-5 text-slate-400" />
              </div>
              <button 
                type="submit"
                disabled={loading}
                className="bg-emerald-600 hover:bg-emerald-700 text-white px-8 rounded-lg font-bold transition-colors disabled:opacity-50 disabled:cursor-not-allowed flex items-center gap-2"
              >
                {loading ? <Loader2 className="w-5 h-5 animate-spin" /> : 'CERCA'}
              </button>
            </div>

            {/* Selettore Fonte */}
            <div className="flex flex-col md:flex-row gap-6 pt-4 border-t border-slate-800">
              <div className="flex items-start gap-4 flex-1">
                <div className="flex-1">
                  <label className="text-sm font-semibold text-slate-400 mb-2 block flex items-center gap-2">
                    <Settings className="w-4 h-4" /> Fonte Dati
                  </label>
                  <div className="flex gap-2">
                    <button
                      type="button"
                      onClick={() => setDataSource('tvmaze')}
                      className={`flex-1 py-2 px-3 rounded-md text-sm font-medium transition-all ${
                        dataSource === 'tvmaze' 
                          ? 'bg-emerald-500/20 text-emerald-400 border border-emerald-500/50' 
                          : 'bg-slate-800 text-slate-400 border border-transparent hover:bg-slate-700'
                      }`}
                    >
                      TVMaze (Gratis, Voti Comunità)
                    </button>
                    <button
                      type="button"
                      onClick={() => setDataSource('omdb')}
                      className={`flex-1 py-2 px-3 rounded-md text-sm font-medium transition-all ${
                        dataSource === 'omdb' 
                          ? 'bg-yellow-500/20 text-yellow-400 border border-yellow-500/50' 
                          : 'bg-slate-800 text-slate-400 border border-transparent hover:bg-slate-700'
                      }`}
                    >
                      OMDb (Voti Reali IMDb)
                    </button>
                  </div>
                </div>
              </div>

              {/* Input API Key per OMDb */}
              {dataSource === 'omdb' && (
                <div className="flex-1 animate-in slide-in-from-top-2 fade-in">
                  <label className="text-sm font-semibold text-slate-400 mb-2 block flex items-center gap-2">
                    <Key className="w-4 h-4" /> OMDb API Key
                  </label>
                  <div className="flex gap-2">
                    <input
                      type="text"
                      value={omdbKey}
                      onChange={handleKeyChange}
                      placeholder="Incolla qui la tua chiave (es. a1b2c3d4)"
                      className="flex-1 bg-slate-800 border border-slate-700 rounded-md py-2 px-3 text-sm text-white focus:ring-1 focus:ring-yellow-500 focus:border-yellow-500 outline-none"
                    />
                    <a 
                      href="https://www.omdbapi.com/apikey.aspx" 
                      target="_blank" 
                      rel="noopener noreferrer"
                      className="px-3 py-2 bg-slate-700 hover:bg-slate-600 rounded-md text-xs text-slate-300 flex items-center text-center whitespace-nowrap transition-colors"
                    >
                      Ottieni Key<br/>Gratis
                    </a>
                  </div>
                  <p className="text-[10px] text-slate-500 mt-1">
                    Necessaria per accedere ai dati IMDb. La chiave viene salvata solo nel tuo browser.
                  </p>
                </div>
              )}
            </div>
          </form>

          {/* Feedback Caricamento e Errori */}
          {loading && (
            <div className="mt-4 p-3 bg-blue-900/20 border border-blue-800/50 rounded-lg text-blue-200 text-sm flex items-center gap-3 animate-pulse">
              <Loader2 className="w-4 h-4 animate-spin" />
              {progress}
            </div>
          )}

          {error && (
            <div className="mt-4 p-4 bg-red-900/50 border border-red-700 text-red-200 rounded-lg flex items-center gap-2 animate-in fade-in slide-in-from-top-2">
              <AlertCircle className="w-5 h-5 shrink-0" />
              <span>{error}</span>
            </div>
          )}
        </div>
      </div>

      {/* Contenuto Principale */}
      {seriesData && !loading && (
        <div className="max-w-7xl mx-auto grid grid-cols-1 lg:grid-cols-[300px_1fr] gap-8 animate-in fade-in duration-500">
          
          {/* Colonna Sinistra: Info Serie */}
          <div className="space-y-4">
            <div className="relative group">
              {seriesData.image ? (
                <img 
                  src={seriesData.image} 
                  alt={seriesData.name} 
                  className="w-full rounded-xl shadow-2xl shadow-black/50 aspect-[2/3] object-cover"
                />
              ) : (
                <div className="w-full aspect-[2/3] bg-slate-800 rounded-xl flex items-center justify-center text-slate-500">
                  Nessuna immagine
                </div>
              )}
              <div className="absolute top-4 right-4 bg-black/80 backdrop-blur-md px-3 py-1.5 rounded-full flex items-center gap-1.5 border border-slate-700 shadow-lg">
                <Star className="w-4 h-4 text-yellow-400 fill-yellow-400" />
                <span className="font-bold text-white">
                  {seriesData.rating || "N/A"}
                </span>
              </div>
            </div>

            <div>
              <h2 className="text-3xl font-bold leading-tight flex items-center gap-2">
                {seriesData.name}
              </h2>
              <span className="text-xs text-slate-500 uppercase tracking-wider font-bold mt-1 block">
                Fonte: {seriesData.source}
              </span>

              <div className="flex flex-wrap gap-2 mt-3">
                {seriesData.genres?.map(g => (
                  <span key={g} className="text-xs font-semibold px-2.5 py-1 bg-slate-800 text-slate-300 rounded-md border border-slate-700">
                    {g}
                  </span>
                ))}
              </div>
              <div 
                className="mt-4 text-slate-400 text-sm leading-relaxed"
              >
                {seriesData.summary?.replace(/<[^>]*>?/gm, '')}
              </div>
            </div>
          </div>

          {/* Colonna Destra: Griglia Episodi */}
          <div className="overflow-hidden">
            
            {/* Legenda */}
            <div className="flex flex-wrap gap-x-6 gap-y-2 mb-6 text-sm justify-end md:justify-start">
              {RATING_COLORS.map((item) => (
                <div key={item.label} className="flex items-center gap-2">
                  <div className={`w-3 h-3 rounded-full ${item.color.split(' ')[0]}`}></div>
                  <span className="text-slate-400 text-xs">{item.label}</span>
                </div>
              ))}
            </div>

            {/* Container Griglia */}
            <div className="overflow-x-auto pb-4 custom-scrollbar">
              <div className="inline-block min-w-full">
                
                {/* Header Stagioni */}
                <div className="flex mb-2">
                  <div className="w-10 shrink-0"></div>
                  {Object.keys(episodes).map(seasonNum => (
                    <div key={`h-s${seasonNum}`} className="w-12 text-center text-slate-500 font-bold text-sm shrink-0">
                      S{seasonNum}
                    </div>
                  ))}
                </div>

                {/* Righe Episodi */}
                {range(maxEpisodes).map((epIndex) => {
                  const epNum = epIndex + 1;
                  return (
                    <div key={`row-${epNum}`} className="flex mb-1 group hover:bg-white/5 transition-colors rounded-lg">
                      <div className="w-10 shrink-0 flex items-center justify-center text-slate-600 text-xs font-mono font-bold">
                        E{epNum}
                      </div>

                      {Object.keys(episodes).map(seasonNum => {
                        const seasonEps = episodes[seasonNum];
                        const episode = seasonEps.find(e => e.number === epNum);
                        const rating = episode?.rating;

                        return (
                          <div key={`cell-${seasonNum}-${epNum}`} className="w-12 h-10 p-0.5 shrink-0">
                            {episode ? (
                              <div 
                                className={`w-full h-full rounded flex items-center justify-center text-xs font-bold shadow-sm transition-all hover:scale-110 hover:z-10 cursor-help ${getRatingColor(rating)}`}
                                title={`S${seasonNum} E${epNum}: ${episode.title} (${rating || 'N/A'})`}
                              >
                                {rating ? rating.toFixed(1) : ''}
                              </div>
                            ) : (
                              <div className="w-full h-full opacity-0"></div>
                            )}
                          </div>
                        );
                      })}
                    </div>
                  );
                })}
              </div>
            </div>
            {dataSource === 'tvmaze' && (
              <p className="flex items-center justify-center gap-2 text-center text-slate-500 text-xs mt-6 bg-slate-900 py-2 rounded-lg border border-slate-800">
                <Info className="w-4 h-4" />
                Nota: Stai visualizzando i voti di TVMaze. Per i voti ufficiali IMDb, seleziona "OMDb" e inserisci una chiave.
              </p>
            )}
          </div>
        </div>
      )}

      {!seriesData && !loading && !error && (
        <div className="flex flex-col items-center justify-center mt-20 text-slate-600">
          <Film className="w-16 h-16 mb-4 opacity-20" />
          <p>Cerca una serie per visualizzare la Series Graph</p>
          <p className="text-sm mt-2 opacity-50">Prova con "Game of Thrones" o "Breaking Bad"</p>
        </div>
      )}
      
      <style>{`
        .custom-scrollbar::-webkit-scrollbar {
          height: 10px;
        }
        .custom-scrollbar::-webkit-scrollbar-track {
          background: #0f172a;
          border-radius: 5px;
        }
        .custom-scrollbar::-webkit-scrollbar-thumb {
          background: #334155;
          border-radius: 5px;
          border: 2px solid #0f172a;
        }
        .custom-scrollbar::-webkit-scrollbar-thumb:hover {
          background: #475569;
        }
      `}</style>
    </div>
  );
};

export default SeriesRatingsApp;