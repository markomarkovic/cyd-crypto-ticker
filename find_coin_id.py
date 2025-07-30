#!/usr/bin/env python3
"""
CoinMarketCap ID Finder
Find coin IDs by symbol/abbreviation for use in crypto ticker web configuration

Usage:
    python find_coin_id.py --api-key YOUR_KEY ETH
    python find_coin_id.py --api-key YOUR_KEY BTC SOL ADA
    python find_coin_id.py --api-key YOUR_KEY --list-top 20
"""

import requests
import sys
import argparse
import json
from typing import List, Dict

def find_coin_by_symbol(symbol: str, api_key: str) -> List[Dict]:
    """Find coin information by symbol"""
    url = "https://pro-api.coinmarketcap.com/v1/cryptocurrency/map"
    headers = {
        'X-CMC_PRO_API_KEY': api_key,
        'Accept': 'application/json'
    }
    
    try:
        response = requests.get(url, headers=headers)
        response.raise_for_status()
        data = response.json()
        
        # Filter by symbol (case insensitive)
        matches = []
        for coin in data.get('data', []):
            if coin['symbol'].upper() == symbol.upper():
                matches.append({
                    'id': coin['id'],
                    'name': coin['name'],
                    'symbol': coin['symbol'],
                    'slug': coin['slug'],
                    'is_active': coin.get('is_active', 1),
                    'rank': coin.get('rank', 999999)
                })
        
        # Sort by rank (lower is better) and active status
        matches.sort(key=lambda x: (not x['is_active'], x['rank']))
        return matches
        
    except requests.RequestException as e:
        print(f"Error fetching data: {e}")
        return []

def get_top_coins(limit: int, api_key: str) -> List[Dict]:
    """Get top N coins by market cap"""
    url = "https://pro-api.coinmarketcap.com/v1/cryptocurrency/listings/latest"
    headers = {
        'X-CMC_PRO_API_KEY': api_key,
        'Accept': 'application/json'
    }
    params = {
        'limit': limit,
        'convert': 'USD'
    }
    
    try:
        response = requests.get(url, headers=headers, params=params)
        response.raise_for_status()
        data = response.json()
        
        coins = []
        for coin in data.get('data', []):
            coins.append({
                'id': coin['id'],
                'name': coin['name'],
                'symbol': coin['symbol'],
                'slug': coin['slug'],
                'rank': coin['cmc_rank'],
                'price': coin['quote']['USD']['price']
            })
        
        return coins
        
    except requests.RequestException as e:
        print(f"Error fetching data: {e}")
        return []

def main():
    parser = argparse.ArgumentParser(
        description='Find CoinMarketCap IDs by coin symbol',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python find_coin_id.py --api-key YOUR_KEY ETH           # Find Ethereum
  python find_coin_id.py --api-key YOUR_KEY BTC SOL ADA   # Find multiple coins
  python find_coin_id.py --api-key YOUR_KEY --list-top 20 # Show top 20 coins
        """
    )
    
    parser.add_argument('symbols', nargs='*', help='Coin symbols to search for (e.g., ETH, BTC)')
    parser.add_argument('--list-top', type=int, metavar='N', 
                       help='List top N coins by market cap')
    parser.add_argument('--api-key', required=True, help='CoinMarketCap API key (required)')
    
    args = parser.parse_args()
    
    # Get API key from command line argument
    api_key = args.api_key
    
    # List top coins
    if args.list_top:
        print(f"Top {args.list_top} cryptocurrencies:")
        print("=" * 60)
        print(f"{'Rank':<5} {'ID':<6} {'Symbol':<8} {'Name':<25} {'Price':<12}")
        print("-" * 60)
        
        coins = get_top_coins(args.list_top, api_key)
        for coin in coins:
            price_str = f"${coin['price']:,.2f}" if coin['price'] > 0.01 else f"${coin['price']:.6f}"
            print(f"{coin['rank']:<5} {coin['id']:<6} {coin['symbol']:<8} {coin['name']:<25} {price_str:<12}")
        
        if coins:
            ids = ",".join(str(coin['id']) for coin in coins[:10])  # First 10 for web config
            print(f"\nFirst 10 IDs for web configuration: {ids}")
        return
    
    # Search for specific symbols
    if not args.symbols:
        parser.print_help()
        return
    
    all_ids = []
    for symbol in args.symbols:
        print(f"\nSearching for '{symbol.upper()}':")
        print("=" * 40)
        
        matches = find_coin_by_symbol(symbol, api_key)
        
        if not matches:
            print(f"❌ No coins found for symbol '{symbol}'")
            continue
        
        # Show all matches
        for i, coin in enumerate(matches):
            status = "✅ ACTIVE" if coin['is_active'] else "❌ INACTIVE"
            rank_str = f"Rank #{coin['rank']}" if coin['rank'] != 999999 else "Unranked"
            
            print(f"{i+1}. {coin['name']} ({coin['symbol']})")
            print(f"   ID: {coin['id']} | {status} | {rank_str}")
            print(f"   Slug: {coin['slug']}")
            
            if i == 0:  # Add the primary match to our list
                all_ids.append(coin['id'])
        
        # Recommend the best match
        if matches:
            best = matches[0]
            print(f"\n💡 Recommended: {best['name']} (ID: {best['id']})")
    
    # Show combined IDs for web configuration
    if all_ids:
        ids_str = ",".join(map(str, all_ids))
        print(f"\n🔗 Combined IDs for web configuration:")
        print(f"Coin IDs: {ids_str}")

if __name__ == "__main__":
    main()