#!/usr/bin/env python3
"""
find_binance_symbols.py - Utility to discover Binance trading pairs for CYD Crypto Ticker

This script helps you find valid Binance trading pairs to configure in your CYD device.
No API key required - uses Binance's public API endpoints.

Usage:
    python find_binance_symbols.py BTC ETH SOL                     # Find specific symbols
    python find_binance_symbols.py --list-top 20                   # List top 20 by volume
    python find_binance_symbols.py --search bitcoin                # Search by name
    python find_binance_symbols.py --usdt-pairs                    # Show all USDT pairs
"""

import requests
import json
import argparse
import sys
from typing import List, Dict, Any

class BinanceSymbolFinder:
    def __init__(self):
        self.base_url = "https://api.binance.com/api/v3"
        self.session = requests.Session()
        self.session.headers.update({
            'User-Agent': 'CYD-Crypto-Ticker/1.0'
        })

    def get_exchange_info(self) -> Dict[str, Any]:
        """Get exchange information including all trading pairs"""
        try:
            response = self.session.get(f"{self.base_url}/exchangeInfo", timeout=10)
            response.raise_for_status()
            return response.json()
        except requests.RequestException as e:
            print(f"Error fetching exchange info: {e}")
            return {}

    def get_24hr_ticker(self) -> List[Dict[str, Any]]:
        """Get 24hr ticker statistics for all trading pairs"""
        try:
            response = self.session.get(f"{self.base_url}/ticker/24hr", timeout=10)
            response.raise_for_status()
            return response.json()
        except requests.RequestException as e:
            print(f"Error fetching ticker data: {e}")
            return []

    def find_symbols(self, symbols: List[str]) -> None:
        """Find specific trading pairs"""
        print(f"Searching for symbols: {', '.join(symbols)}")
        print("-" * 50)
        
        exchange_info = self.get_exchange_info()
        if not exchange_info:
            return
            
        trading_pairs = exchange_info.get('symbols', [])
        found_pairs = []
        
        for symbol_search in symbols:
            symbol_upper = symbol_search.upper()
            matches = []
            
            # Look for exact matches and partial matches
            for pair in trading_pairs:
                if pair['status'] != 'TRADING':
                    continue
                    
                symbol = pair['symbol']
                base = pair['baseAsset']
                quote = pair['quoteAsset']
                
                # Exact symbol match
                if symbol == symbol_upper:
                    matches.append((symbol, base, quote, 'exact'))
                # Base asset match with USDT
                elif base == symbol_upper and quote == 'USDT':
                    matches.append((symbol, base, quote, 'usdt_pair'))
                # Base asset match with other quotes
                elif base == symbol_upper:
                    matches.append((symbol, base, quote, 'other_pair'))
            
            if matches:
                print(f"\nFound matches for '{symbol_search}':")
                # Sort by match type priority
                matches.sort(key=lambda x: {'exact': 0, 'usdt_pair': 1, 'other_pair': 2}[x[3]])
                
                for symbol, base, quote, match_type in matches[:10]:  # Limit to 10 matches
                    found_pairs.append(symbol)
                    match_desc = {
                        'exact': 'Exact match',
                        'usdt_pair': 'USDT pair', 
                        'other_pair': 'Other pair'
                    }[match_type]
                    print(f"  {symbol:<15} ({base}/{quote}) - {match_desc}")
            else:
                print(f"\nNo matches found for '{symbol_search}'")
        
        if found_pairs:
            # Filter for USDT pairs only (recommended for CYD)
            usdt_pairs = [pair for pair in found_pairs if pair.endswith('USDT')]
            if usdt_pairs:
                print(f"\n📝 Recommended USDT pairs for CYD configuration:")
                print(f"   {','.join(usdt_pairs[:6])}")  # Limit to 6 pairs
                if len(usdt_pairs) > 6:
                    print(f"   (Limited to 6 pairs - CYD maximum)")

    def list_top_pairs(self, count: int = 20) -> None:
        """List top trading pairs by 24hr volume"""
        print(f"Top {count} trading pairs by 24hr volume:")
        print("-" * 50)
        
        ticker_data = self.get_24hr_ticker()
        if not ticker_data:
            return
            
        # Filter for USDT pairs and sort by volume
        usdt_pairs = [
            ticker for ticker in ticker_data 
            if ticker['symbol'].endswith('USDT')
        ]
        
        # Sort by quote volume (USDT volume)
        usdt_pairs.sort(key=lambda x: float(x['quoteVolume']), reverse=True)
        
        print(f"{'Symbol':<15} {'Price (USDT)':<15} {'24h Change':<15} {'Volume (USDT)':<20}")
        print("-" * 75)
        
        symbols_for_config = []
        for i, ticker in enumerate(usdt_pairs[:count]):
            symbol = ticker['symbol']
            price = float(ticker['lastPrice'])
            change_percent = float(ticker['priceChangePercent'])
            volume = float(ticker['quoteVolume'])
            
            # Format volume in millions/billions
            if volume >= 1e9:
                volume_str = f"{volume/1e9:.1f}B"
            elif volume >= 1e6:
                volume_str = f"{volume/1e6:.1f}M" 
            else:
                volume_str = f"{volume:,.0f}"
                
            change_sign = "+" if change_percent >= 0 else ""
            
            print(f"{symbol:<15} {price:<15,.2f} {change_sign}{change_percent:<14.2f}% {volume_str:<20}")
            
            if i < 6:  # Collect first 6 for configuration
                symbols_for_config.append(symbol)
        
        print(f"\n📝 Top 6 pairs for CYD configuration:")
        print(f"   {','.join(symbols_for_config)}")

    def search_by_name(self, search_term: str) -> None:
        """Search for trading pairs by cryptocurrency name"""
        print(f"Searching for cryptocurrencies matching '{search_term}':")
        print("-" * 50)
        
        # Get ticker data to show current prices
        ticker_data = self.get_24hr_ticker()
        ticker_dict = {ticker['symbol']: ticker for ticker in ticker_data}
        
        exchange_info = self.get_exchange_info()
        if not exchange_info:
            return
            
        trading_pairs = exchange_info.get('symbols', [])
        search_lower = search_term.lower()
        matches = []
        
        # Common cryptocurrency name mappings
        name_mappings = {
            'bitcoin': 'BTC', 'ethereum': 'ETH', 'binance': 'BNB', 'cardano': 'ADA',
            'solana': 'SOL', 'dogecoin': 'DOGE', 'polkadot': 'DOT', 'avalanche': 'AVAX',
            'chainlink': 'LINK', 'polygon': 'MATIC', 'uniswap': 'UNI', 'litecoin': 'LTC',
            'stellar': 'XLM', 'algorand': 'ALGO', 'cosmos': 'ATOM', 'filecoin': 'FIL'
        }
        
        # Check if search term matches a known name
        base_assets = set()
        if search_lower in name_mappings:
            base_assets.add(name_mappings[search_lower])
        
        # Also search for partial matches in base assets
        for pair in trading_pairs:
            if pair['status'] != 'TRADING':
                continue
            base = pair['baseAsset']
            if search_lower in base.lower():
                base_assets.add(base)
        
        # Find USDT pairs for matching base assets
        for pair in trading_pairs:
            if pair['status'] != 'TRADING':
                continue
            if pair['baseAsset'] in base_assets and pair['quoteAsset'] == 'USDT':
                symbol = pair['symbol']
                ticker = ticker_dict.get(symbol, {})
                matches.append((symbol, pair['baseAsset'], ticker))
        
        if matches:
            # Sort by volume if available
            matches.sort(key=lambda x: float(x[2].get('quoteVolume', 0)), reverse=True)
            
            print(f"{'Symbol':<15} {'Base Asset':<12} {'Price (USDT)':<15} {'24h Change':<15}")
            print("-" * 65)
            
            symbols_for_config = []
            for i, (symbol, base, ticker) in enumerate(matches[:20]):
                price = float(ticker.get('lastPrice', 0))
                change_percent = float(ticker.get('priceChangePercent', 0))
                change_sign = "+" if change_percent >= 0 else ""
                
                print(f"{symbol:<15} {base:<12} {price:<15,.4f} {change_sign}{change_percent:<14.2f}%")
                
                if i < 6:
                    symbols_for_config.append(symbol)
            
            if symbols_for_config:
                print(f"\n📝 Matching pairs for CYD configuration:")
                print(f"   {','.join(symbols_for_config)}")
        else:
            print(f"No USDT trading pairs found matching '{search_term}'")

    def show_usdt_pairs(self) -> None:
        """Show all available USDT trading pairs"""
        print("All available USDT trading pairs:")
        print("-" * 50)
        
        exchange_info = self.get_exchange_info()
        if not exchange_info:
            return
            
        trading_pairs = exchange_info.get('symbols', [])
        usdt_pairs = [
            pair['symbol'] for pair in trading_pairs 
            if pair['status'] == 'TRADING' and pair['quoteAsset'] == 'USDT'
        ]
        
        usdt_pairs.sort()
        
        print(f"Total USDT pairs available: {len(usdt_pairs)}")
        print("\nPairs (showing first 100):")
        
        # Display in columns
        for i in range(0, min(100, len(usdt_pairs)), 4):
            row = usdt_pairs[i:i+4]
            print("  ".join(f"{pair:<15}" for pair in row))
        
        if len(usdt_pairs) > 100:
            print(f"\n... and {len(usdt_pairs) - 100} more pairs")
        
        # Suggest popular pairs
        popular_pairs = [pair for pair in usdt_pairs if any(
            pair.startswith(symbol) for symbol in 
            ['BTC', 'ETH', 'BNB', 'ADA', 'SOL', 'DOGE', 'MATIC', 'DOT', 'AVAX', 'LINK']
        )]
        
        if popular_pairs:
            print(f"\n📝 Popular pairs for CYD configuration:")
            print(f"   {','.join(popular_pairs[:6])}")

def main():
    parser = argparse.ArgumentParser(
        description='Find Binance trading pairs for CYD Crypto Ticker configuration',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python find_binance_symbols.py BTC ETH SOL                    # Find specific symbols
  python find_binance_symbols.py --list-top 20                  # List top 20 by volume  
  python find_binance_symbols.py --search bitcoin               # Search by name
  python find_binance_symbols.py --usdt-pairs                   # Show all USDT pairs
        """
    )
    
    parser.add_argument('symbols', nargs='*', help='Cryptocurrency symbols to find')
    parser.add_argument('--list-top', type=int, metavar='N', 
                       help='List top N trading pairs by volume')
    parser.add_argument('--search', metavar='TERM', 
                       help='Search for trading pairs by cryptocurrency name')
    parser.add_argument('--usdt-pairs', action='store_true',
                       help='Show all available USDT trading pairs')
    
    args = parser.parse_args()
    
    finder = BinanceSymbolFinder()
    
    if args.list_top:
        finder.list_top_pairs(args.list_top)
    elif args.search:
        finder.search_by_name(args.search)
    elif args.usdt_pairs:
        finder.show_usdt_pairs()
    elif args.symbols:
        finder.find_symbols(args.symbols)
    else:
        # Default: show top pairs
        finder.list_top_pairs()

if __name__ == "__main__":
    main()